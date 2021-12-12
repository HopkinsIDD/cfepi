#include <catch2/catch.hpp>


#include <cfepi/sir.hpp>
#include <cfepi/modeling.hpp>
#include <iostream>
#include <range/v3/all.hpp>

// TEST OF SIR

namespace detail {
struct sir_epidemic_states {
public:
  enum state { S, I, R, n_compartments };
  constexpr auto size() const { return (static_cast<size_t>(n_compartments)); }
};

struct sir_recovery_event : public cfepi::transition_event<sir_epidemic_states> {
  constexpr sir_recovery_event(cfepi::person_t p1, cfepi::epidemic_time_t _time) noexcept
    : transition_event<sir_epidemic_states>(p1,
      _time,
      { std::bitset<std::size(sir_epidemic_states{})>{
        1 << sir_epidemic_states::I} },
      sir_epidemic_states::R){};
  constexpr sir_recovery_event() noexcept : sir_recovery_event(0UL, -1) {};
};

struct sir_infection_event : public cfepi::interaction_event<sir_epidemic_states> {
  constexpr sir_infection_event(cfepi::person_t p1, cfepi::person_t p2, cfepi::epidemic_time_t _time) noexcept
    : interaction_event<sir_epidemic_states>(p1,
      p2,
      _time,
      { std::bitset<std::size(sir_epidemic_states{})>{
        1 << sir_epidemic_states::S} },
      { std::bitset<std::size(sir_epidemic_states{})>{
        1 << sir_epidemic_states::I} },
      sir_epidemic_states::I){};
  constexpr sir_infection_event() noexcept : sir_infection_event(0UL, 0UL, -1) {};
};

typedef std::variant<sir_recovery_event, sir_infection_event> any_sir_event;

TEST_CASE("SIR model works modularly", "[sir_generator]") {
  cfepi::person_t population_size = 10000;
  auto initial_conditions = cfepi::default_state<sir_epidemic_states>(
    sir_epidemic_states::S, sir_epidemic_states::I, population_size, 1UL);
  auto always_true = [](const auto &param __attribute__((unused)),
                       std::default_random_engine &rng __attribute__((unused))) { return (true); };
  cfepi::run_simulation<sir_epidemic_states, any_sir_event>(initial_conditions,
    std::array<double, 2>({ .1, 2. / static_cast<double>(population_size) }),
    { always_true, always_true });
}

TEST_CASE("Single Time Event Generator works as expected") {
  const cfepi::person_t population_size = 5;
  auto initial_conditions = cfepi::default_state<sir_epidemic_states>(sir_epidemic_states::S, sir_epidemic_states::I, population_size, 1UL);
  auto event_range_generator =
    cfepi::single_type_event_generator<sir_epidemic_states, any_sir_event, 1>(initial_conditions);
  auto event_range = event_range_generator.event_range();


  size_t counter = 0;
  cfepi::person_t zero = 0ul;
  const cfepi::person_t one = 1ul;
  for (auto val : event_range) {
    ++counter;
    // any_sir_event_print{"TEST"}(val);
    REQUIRE(val.affected_people[one] == 0ul);
    REQUIRE(val.affected_people[zero] == counter);
  }

  REQUIRE(counter == population_size - 1);

  initial_conditions.potential_states[1][sir_epidemic_states::I] = true;
  event_range_generator = cfepi::single_type_event_generator<sir_epidemic_states, any_sir_event, 1>(initial_conditions);
  event_range = event_range_generator.event_range();

  counter = 0;
  for (auto val : event_range) {
    ++counter;
    REQUIRE(val.affected_people[zero] == ((counter - 1) / 2) + 1);
    if (((counter - 1) % 2) == 0) {
      REQUIRE(val.affected_people[one] == 0ul);
    } else {
      REQUIRE(val.affected_people[one] == 1ul);
    }
  }

  REQUIRE(counter == 2 * (population_size - 1));
}

TEST_CASE("sample_view is working", "[sample_view]") {
  auto gen1 = std::mt19937{ std::random_device{}() };
  auto gen2 = std::mt19937{ gen1 };
  auto gen3 = std::mt19937{ gen1 };
  auto view = std::ranges::views::iota(0, 100000);
  probability::sample_view view1(view, 0.005, gen1);
  auto view2 = view | probability::views::sample(0.005, gen2);
  probability::sample_view view3(view, 0.005, gen3);
  static_assert(std::ranges::forward_range<probability::sample_view<decltype(view), decltype(gen1)>>);
  auto it2 = std::begin(view2);
  for (auto it1 = std::begin(view1); (it1 != std::end(view1)) && (it2 != std::end(view2)); ++it1) {
    REQUIRE((*it1) == (*it2));
    ++it2;
  }
  it2 = std::begin(view2);
  for (auto it1 = std::begin(view3); (it1 != std::end(view3)) && (it2 != std::end(view2)); ++it1) {
    REQUIRE((*it1) == (*it2));
    ++it2;
  }
}

TEST_CASE("sample_view with probability 1 has the same size as original view", "[sample_view]") {
  auto gen = std::mt19937{ std::random_device{}() };
  auto base_view = std::ranges::views::iota(0, 100000);
  probability::sample_view fully_sampled_view(base_view, 1.0 , gen);
  size_t base_counter = 0UL;
  for(auto elem __attribute__((unused)) : base_view ) {
    ++base_counter;
  }
  size_t fully_sampled_counter = 0UL;
  for(auto elem __attribute__((unused)) : fully_sampled_view ) {
    ++fully_sampled_counter;
  }
  REQUIRE(base_counter == fully_sampled_counter);
}

TEST_CASE("States are properly separated", "") {

  struct test_epidemic_states{
  public:
    enum state { S, E, I, R, n_SIR_compartments };
    constexpr auto size() const {
      return(static_cast<size_t>(n_SIR_compartments));
    }
  };

  cfepi::sir_state<test_epidemic_states>{};
}

TEST_CASE("Full stack test works", "[sir_generator]") {
  std::random_device rd;
  size_t global_seed = 2;
  std::default_random_engine random_source_1{ global_seed++ };
  const cfepi::person_t population_size = 100;

  auto always_true = [](const auto &param __attribute__((unused)) ) { return (true); };
  auto always_false = [](const auto &param __attribute__((unused)) ) { return (false); };
  auto initial_conditions = cfepi::default_state<sir_epidemic_states>(sir_epidemic_states::S, sir_epidemic_states::I, population_size, 1UL);

  std::vector<std::function<bool(const any_sir_event &)>> filters{always_true, always_false};

  // BEGIN
  auto current_state = initial_conditions;

  auto setups_by_filter = ranges::to<std::vector>(
    ranges::views::transform(filters, [&current_state](auto &x) {
      return (std::make_tuple(current_state, current_state, current_state, x));
    }));

  for(cfepi::epidemic_time_t t = 0UL; t < 365; ++t){

    current_state.reset();
    current_state = std::transform_reduce(
      std::begin(setups_by_filter),
      std::end(setups_by_filter),
      current_state,
      [](const auto &x, const auto &y) { return (x || y); },
      [](const auto &x) { return (std::get<0>(x)); });
    ranges::for_each(setups_by_filter, [](auto&x){
      std::get<1>(x).reset();
      std::get<2>(x) = std::get<0>(x);
    });

    std::array<double, 2> event_probabilities = {.2, 2. / population_size};

    const auto single_event_type_run = [ &t, &setups_by_filter, &random_source_1, &population_size, &current_state, &event_probabilities](const auto event_index, const size_t seed){
      random_source_1.seed(seed);
      auto event_range_generator =
        cfepi::single_type_event_generator<sir_epidemic_states, any_sir_event, event_index>(current_state);
      auto this_event_range = event_range_generator.event_range();

      auto sampled_event_view =
        this_event_range | probability::views::sample(event_probabilities[event_index], random_source_1);

      size_t counter = 0UL;
      auto setup_view = std::ranges::views::iota(0UL, setups_by_filter.size());
      auto view_to_filter = ranges::views::cartesian_product(setup_view, sampled_event_view);
      auto filtered_view = ranges::filter_view(view_to_filter, [setups_by_filter = std::as_const(setups_by_filter)](const auto &x) {
        auto filter = std::get<3>(setups_by_filter[std::get<0>(x)]);
        any_sir_event event = std::get<1>(x);
        return (filter(event));
      });

      ranges::for_each(filtered_view, [&setups_by_filter, &counter](const auto&x) {
	if (cfepi::any_state_check_preconditions<any_sir_event, sir_epidemic_states>{std::get<0>(setups_by_filter[std::get<0>(x)])}(std::get<1>(x))) {
	  cfepi::any_event_apply_entered_states{std::get<1>(setups_by_filter[std::get<0>(x)])}(std::get<1>(x));
	  cfepi::any_event_apply_left_states{std::get<2>(setups_by_filter[std::get<0>(x)])}(std::get<1>(x));
	}
      });
      // std::cout << counter << std::endl;

      ranges::for_each(setups_by_filter, [& t](auto&x){
        std::get<0>(x) = std::get<1>(x) || std::get<2>(x);
        std::get<0>(x).time = t;
        // print(std::get<0>(x));
      });
    };

    single_event_type_run(std::integral_constant<size_t, 0UL>(), global_seed++);
    single_event_type_run(std::integral_constant<size_t, 1UL>(), global_seed++);
  }

}

// TEST OF SEIR

struct seir_epidemic_states {
public:
  enum state { S, E, I, R, n_compartments };
  constexpr auto size() const { return (static_cast<size_t>(n_compartments)); }
};

struct seir_exposure_event : public cfepi::interaction_event<seir_epidemic_states> {
  constexpr seir_exposure_event(cfepi::person_t p1, cfepi::person_t p2, cfepi::epidemic_time_t _time) noexcept
    : interaction_event<seir_epidemic_states>(p1,
      p2,
      _time,
      { std::bitset<std::size(seir_epidemic_states{})>{
        1 << seir_epidemic_states::S} },
      { std::bitset<std::size(seir_epidemic_states{})>{
        1 << seir_epidemic_states::I} },
      seir_epidemic_states::E){};
  constexpr seir_exposure_event() noexcept : seir_exposure_event(0UL, 0UL, -1) {};
};

struct seir_recovery_event : public cfepi::transition_event<seir_epidemic_states> {
  constexpr seir_recovery_event(cfepi::person_t p1, cfepi::epidemic_time_t _time) noexcept
    : transition_event<seir_epidemic_states>(p1,
      _time,
      { std::bitset<std::size(seir_epidemic_states{})>{
        1 << seir_epidemic_states::I} },
      seir_epidemic_states::R){};
  constexpr seir_recovery_event() noexcept : seir_recovery_event(0UL, -1) {};
};

struct seir_infection_event : public cfepi::transition_event<seir_epidemic_states> {
  constexpr seir_infection_event(cfepi::person_t p1, cfepi::epidemic_time_t _time) noexcept
    : transition_event<seir_epidemic_states>(p1,
      _time,
      { std::bitset<std::size(seir_epidemic_states{})>{
        1 << seir_epidemic_states::E} },
      seir_epidemic_states::I){};
  constexpr seir_infection_event() noexcept : seir_infection_event(0UL, -1) {};
};

typedef std::variant<seir_recovery_event, seir_infection_event, seir_exposure_event> any_seir_event;

TEST_CASE("SEIR model works modularly", "[sir_generator]") {
  cfepi::person_t population_size = 10000;
  auto initial_conditions = cfepi::default_state<seir_epidemic_states>(
    seir_epidemic_states::S, seir_epidemic_states::I, population_size, 1UL);
  auto always_true = [](const auto &param __attribute__((unused)), std::default_random_engine& rng __attribute__((unused))) { return (true); };
  cfepi::run_simulation<seir_epidemic_states, any_seir_event>(initial_conditions,
    std::array<double, 3>({ .1, .8, 2. / static_cast<double>(population_size) }),
    { always_true, always_true });
}
}

#include <catch2/catch.hpp>
#include <cfepi/sample_view.hpp>
#include <range/v3/all.hpp>

#include <cfepi/sir.hpp>
#include <cfepi/modeling.hpp>

struct seir_epidemic_states {
public:
  enum state { S, E, I, R, n_compartments };
  constexpr auto size() const { return (static_cast<size_t>(n_compartments)); }
};


size_t global_event_counter = 0;

struct exposure_event : public interaction_event<seir_epidemic_states> {
  constexpr exposure_event() noexcept
    : interaction_event<seir_epidemic_states>({ true, false, false, false },
      { false, false, true, false },
      seir_epidemic_states::E){};
  constexpr exposure_event(person_t p1, person_t p2, epidemic_time_t _time) noexcept
    : interaction_event<seir_epidemic_states>(p1,
      p2,
      _time,
      { true, false, false, false },
      { false, false, true, false },
      seir_epidemic_states::E){};
};

struct recovery_event : public transition_event<seir_epidemic_states> {
  constexpr recovery_event() noexcept
    : transition_event<seir_epidemic_states>({ false, false, true, false },
      seir_epidemic_states::R){};
  constexpr recovery_event(person_t p1, epidemic_time_t _time) noexcept
    : transition_event<seir_epidemic_states>(p1,
      _time,
      { false, false, true, false },
      seir_epidemic_states::R){};
};

struct infection_event : public transition_event<seir_epidemic_states> {
  constexpr infection_event() noexcept
    : transition_event<seir_epidemic_states>({ false, true, false, false },
      seir_epidemic_states::I){};
  constexpr infection_event(person_t p1, epidemic_time_t _time) noexcept
    : transition_event<seir_epidemic_states>(p1,
      _time,
      { false, true, false, false },
      seir_epidemic_states::I){};
};

typedef std::variant<recovery_event, infection_event, exposure_event> any_seir_event;


template<> struct event_true_preconditions<any_seir_event, 0, 0> {
  constexpr static auto value = { seir_epidemic_states::I };
};

template<> struct event_true_preconditions<any_seir_event, 1, 0> {
  constexpr static auto value = { seir_epidemic_states::E };
};

template<> struct event_true_preconditions<any_seir_event, 2, 0> {
  constexpr static auto value = { seir_epidemic_states::S };
};

template<> struct event_true_preconditions<any_seir_event, 2, 1> {
  constexpr static auto value = { seir_epidemic_states::I };
};

template<> struct event_by_index<any_seir_event, 0> : recovery_event {};

template<> struct event_by_index<any_seir_event, 1> : infection_event {};

template<> struct event_by_index<any_seir_event, 2> : exposure_event {};

/*
TEST_CASE("SEIR model works", "[sir_generator]") {
  std::random_device rd;
  size_t global_seed = 2;
  std::default_random_engine random_source_1{ global_seed++ };
  const person_t population_size = 10000;
  std::array<double, 3> event_probabilities = { .1, .8, 2. / population_size };

  auto always_true = [](const auto &param __attribute__((unused))) { return (true); };
  auto always_false = [](const auto &param __attribute__((unused))) { return (true); };

  auto initial_conditions = default_state<seir_epidemic_states>(
    seir_epidemic_states::S, seir_epidemic_states::I, population_size, 1UL);

  std::vector<std::function<bool(const any_seir_event &)>> filters{ always_true, always_false };

  // BEGIN
  auto current_state = initial_conditions;

  auto setups_by_filter =
    ranges::to<std::vector>(ranges::views::transform(filters, [&current_state](auto &x) {
      return (std::make_tuple(current_state, current_state, current_state, x));
    }));

  for (epidemic_time_t t = 0UL; t < 365; ++t) {

    current_state.reset();
    current_state = std::transform_reduce(
      std::begin(setups_by_filter),
      std::end(setups_by_filter),
      current_state,
      [](const auto &x, const auto &y) { return (x || y); },
      [](const auto &x) { return (std::get<0>(x)); });
    ranges::for_each(setups_by_filter, [](auto &x) {
      std::get<1>(x).reset();
      std::get<2>(x) = std::get<0>(x);
    });


    const auto single_event_type_run = [&t,
                                         &setups_by_filter,
                                         &random_source_1,
                                         &population_size,
                                         &current_state,
                                         &event_probabilities](
                                         const auto event_index, const size_t seed) {
      random_source_1.seed(seed);
      auto event_range_generator =
        single_type_event_generator<seir_epidemic_states, event_index>(current_state);
      auto this_event_range = event_range_generator.event_range();

      auto sampled_event_view =
        this_event_range
        | probability::views::sample(event_probabilities[event_index], random_source_1);

      size_t counter = 0UL;
      auto setup_view = std::ranges::views::iota(0UL, setups_by_filter.size());
      auto view_to_filter = ranges::views::cartesian_product(setup_view, sampled_event_view);
      auto filtered_view = ranges::filter_view(
        view_to_filter, [setups_by_filter = std::as_const(setups_by_filter)](const auto &x) {
          auto filter = std::get<3>(setups_by_filter[std::get<0>(x)]);
          any_seir_event event = std::get<1>(x);
          return (filter(event));
        });

      ranges::for_each(filtered_view, [&setups_by_filter, &counter](const auto &x) {
        if (any_sir_state_check_preconditions{ std::get<0>(setups_by_filter[std::get<0>(x)]) }(
              std::get<1>(x))) {
          any_event_apply_entered_states{ std::get<1>(setups_by_filter[std::get<0>(x)]) }(
            std::get<1>(x));
          any_event_apply_left_states{ std::get<2>(setups_by_filter[std::get<0>(x)]) }(
            std::get<1>(x));
        }
	++counter;
      });
      global_event_counter += counter;
      std::cout << "Processed " << counter << " events of type " << event_index << " and " << global_event_counter << " total" << std::endl;

      ranges::for_each(setups_by_filter, [&t](auto &x) {
        std::get<0>(x) = std::get<1>(x) || std::get<2>(x);
        std::get<0>(x).time = t;
        print(std::get<0>(x));
      });
    };

    const auto seeded_single_event_type_run = [&single_event_type_run, &global_seed](
                                                const auto event_index) {
      ++global_seed;
      single_event_type_run(event_index, global_seed);
    };

    cfor::constexpr_for<0, 3, 1>(seeded_single_event_type_run);
  }
}
*/

TEST_CASE("SEIR model works modularly", "[sir_generator]") {
  person_t population_size = 10000;
  auto initial_conditions = default_state<seir_epidemic_states>(seir_epidemic_states::S, seir_epidemic_states::I, population_size, 1UL);
  auto always_true = [](const auto &param __attribute__((unused))) { return (true); };
  run_simulation<seir_epidemic_states, any_seir_event>(
								initial_conditions, std::array<double, 3>({ .1, .8, 2. / static_cast<double>(population_size) }), { always_true, always_true });
}

#include <catch2/catch.hpp>

#include <cfepi/sir.hpp>
#include <cfepi/sample_view.hpp>
#include <iostream>
#include <range/v3/all.hpp>

TEST_CASE("Single Time Event Generator works as expected") {
  const person_t population_size = 5;
  auto initial_conditions = default_state(population_size);
  auto event_range_generator =
    single_type_event_generator<1>(initial_conditions);
  auto event_range = event_range_generator.event_range();

  size_t counter = 0;
  person_t zero = 0ul;
  const person_t one = 1ul;
  for (auto val : event_range) {
    ++counter;
    REQUIRE(val.affected_people[one] == 0ul);
    // REQUIRE(std::visit(any_sir_event_affected_people{one},val) == 0ul);
    REQUIRE(val.affected_people[zero] == counter);
    // REQUIRE(std::visit(any_sir_event_affected_people{zero},val) == counter);
  }

  REQUIRE(counter == population_size - 1);

  initial_conditions.potential_states[1][I] = true;
  event_range_generator = single_type_event_generator<1>(initial_conditions);
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

TEST_CASE("", "") {

}

TEST_CASE("Full stack test works", "[sir_generator]") {
  std::random_device rd;
  size_t seed = 2;
  std::default_random_engine random_source_1{ seed++ };
  const person_t population_size = 100;

  auto always_true = [](const auto &param) { return (true); };
  auto always_false = [](const auto &param) { return (false); };
  auto initial_conditions = default_state(population_size);

  std::vector<std::function<bool(const any_sir_event &)>> filters{always_true, always_false};

  // BEGIN
  auto current_state = initial_conditions;

  auto setups_by_filter = ranges::to<std::vector>(
    ranges::views::transform(filters, [&current_state](auto &x) {
      return (std::make_tuple(current_state, current_state, current_state, x));
    }));

  for(epidemic_time_t t = 0UL; t < 365; ++t){

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
        single_type_event_generator<event_index>(current_state);
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
	if (any_sir_state_check_preconditions{std::get<0>(setups_by_filter[std::get<0>(x)])}(std::get<1>(x))) {
	  any_sir_event_apply_entered_states{std::get<1>(setups_by_filter[std::get<0>(x)])}(std::get<1>(x));
	  any_sir_event_apply_left_states{std::get<2>(setups_by_filter[std::get<0>(x)])}(std::get<1>(x));
	}
      });
      std::cout << counter << std::endl;

      ranges::for_each(setups_by_filter, [& t](auto&x){
        std::get<0>(x) = std::get<1>(x) || std::get<2>(x);
        std::get<0>(x).time = t;
        print(std::get<0>(x));
      });
    };

    single_event_type_run(std::integral_constant<size_t, 0UL>(), seed++);
    single_event_type_run(std::integral_constant<size_t, 1UL>(), seed++);
  }

}

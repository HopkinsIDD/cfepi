#include <catch2/catch.hpp>

#include <cfepi/sir.hpp>
#include <iostream>
// #include <stxxl>

TEST_CASE("Cartesian Multiproduct works as expected for a range","[sir_generator]"){
  const person_t population_size = 5;
  auto initial_conditions = default_state(population_size);
  auto event_range_generator =
    cartesian_multiproduct<2>(initial_conditions);
  auto event_range = event_range_generator.event_range();

  size_t counter = 0;
  person_t zero = 0ul;
  person_t one = 1ul;
  for (auto val : event_range) {
    ++counter;
    REQUIRE(std::visit(any_sir_event_affected_people{one},val) == 0ul);
    REQUIRE(std::visit(any_sir_event_affected_people{zero},val) == counter);
  }

  REQUIRE(counter == population_size - 1);

  initial_conditions.potential_states[1][I] = true;
  event_range_generator = cartesian_multiproduct<2>(initial_conditions);
  event_range = event_range_generator.event_range();

  counter = 0;
  for (auto val : event_range) {
    ++counter;
    REQUIRE(std::visit(any_sir_event_affected_people{zero},val) == ((counter - 1) / 2) + 1);
    if (((counter - 1) % 2) == 0) {
      REQUIRE(std::visit(any_sir_event_affected_people{one},val) == 0ul);
    } else {
      REQUIRE(std::visit(any_sir_event_affected_people{one},val) == 1ul);
    }
  }

  REQUIRE(counter == 2 * (population_size - 1));
}

TEST_CASE("Interesting ranges problem", "[ranges]"){
  auto view_1 = ranges::views::iota(0) | ranges::views::take(10);
  auto view_2 = ranges::views::iota(0) | ranges::views::take(100) | ranges::views::sample(10) | ranges::to<std::vector>;
  auto product_view = ranges::cartesian_product_view(view_1, view_2);
  size_t counter = 0;

  for(auto __attribute__((unused)) elem : product_view){
    std::cout << std::get<0>(elem) << ", " << std::get<1>(elem) << "\n";
    ++counter;
  }

  for(auto __attribute__((unused)) elem : product_view){
    std::cout << std::get<0>(elem) << ", " << std::get<1>(elem) << "\n";
  }
  assert(counter == 100);
}

TEST_CASE("Full stack test works","[sir_generator]"){
  std::random_device rd;
  std::default_random_engine random_source_1{1};
  const person_t population_size = 5;
  auto initial_conditions = default_state(population_size);

  // BEGIN
  auto current_state = initial_conditions;
  auto future_states_entered = initial_conditions;
  auto future_states_remained = initial_conditions;

  auto event_range_generator =
    cartesian_multiproduct<2>(current_state);
  auto event_range = event_range_generator.event_range();
  std::vector<std::function<bool(any_sir_event)> > filters;
  filters.push_back(
    [ &future_states_entered, &future_states_remained](auto && __attribute__((unused)) x){
      auto __attribute__((unused)) a = future_states_entered;
      auto __attribute__((unused)) b = future_states_remained;
      return(true);
    }
  );


  size_t counter = 0;

  auto view_1 = ranges::views::sample(event_range, 10, random_source_1) | ranges::to<std::vector>;
  auto __attribute__((unused))view_2 = ranges::views::take(ranges::views::iota(0),2);

  for (auto __attribute__((unused)) x : view_1) {
    ++counter;
      std::cout << counter << std::endl;
  }

  ranges::for_each(
    ranges::filter_view(
      ranges::views::cartesian_product(
        view_1,
	view_2
      ),
      [&current_state](auto&& __attribute__((unused)) x){
	auto __attribute__((unused)) a = current_state;
	std::cout << "HERE" << std::endl;
	return(true);
      }
    ),
    [&counter](auto __attribute__((unused)) x){
      std::cout << counter << std::endl;
      ++counter;
      return;
    }
  );



  REQUIRE(counter > 1);
}

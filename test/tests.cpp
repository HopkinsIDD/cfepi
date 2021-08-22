#include <catch2/catch.hpp>

#include <cfepi/sir.hpp>
#include <cfepi/sample_view.hpp>
#include <iostream>
#include <range/v3/all.hpp>
// #include <stxxl>

TEST_CASE("Single Time Event Generator works as expected") {
  const person_t population_size = 5;
  auto initial_conditions = default_state(population_size);
  auto event_range_generator =
    single_type_event_generator<2>(initial_conditions);
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
  event_range_generator = single_type_event_generator<2>(initial_conditions);
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

/*
TEST_CASE("Interesting ranges problem", "[ranges]"){
  auto gen = std::mt19937{std::random_device{}()};
  auto view_1 = std::ranges::views::iota(0) |
    std::ranges::views::take(10);
  auto view_2 = std::ranges::views::iota(0) |
    std::ranges::views::take(100) |
    probability::views::sample(.1, gen);
  // auto product_view = cor3ntin::rangesnext::product(view_1, view_2);
  size_t counter = 0;

  for(auto elem : view_1){
    std::cout << elem << "\n";
  }
  for(auto elem : view_2){
    std::cout << elem << "\n";
  }
  for(auto __attribute__((unused)) elem : product_view){
    std::cout << std::get<0>(elem) << ", " << std::get<1>(elem) << "\n";
    ++counter;
  }

  for(auto __attribute__((unused)) elem : product_view){
    std::cout << std::get<0>(elem) << ", " << std::get<1>(elem) << "\n";
  }
  assert(counter == 100);
}
*/


TEST_CASE("Full stack test works", "[sir_generator]") {
  std::random_device rd;
  std::default_random_engine random_source_1{ 1 };
  const person_t population_size = 10000;

  auto always_x = [](const auto x){
      return([x](const auto &param) { return (x); });
    };
  auto always_true = [](const auto &param) { return (true); };
  auto always_false = [](const auto &param) { return (false); };
  auto initial_conditions = default_state(population_size);

  std::vector<std::function<bool(const any_sir_event &)>> filters{always_true, always_true};

  // BEGIN
  auto current_state = initial_conditions;
  for(epidemic_time_t t = 0UL; t < 365; ++t){

    auto future_states_entered = current_state;
    future_states_entered.reset();
    auto future_states_remained = ranges::to<std::vector>(ranges::views::transform(filters, always_x(current_state)));

    auto event_range_generator =
      single_type_event_generator<2>(current_state);
    auto event_range = event_range_generator.event_range();

    auto t1 = std::get<0>(event_range_generator.vectors);



    auto sampled_event_view = event_range | probability::views::sample(10./population_size, random_source_1);

    size_t counter = 0;
    ranges::for_each(
		     ranges::filter_view(
					 ranges::views::cartesian_product(
									  ranges::views::enumerate(filters),
									  sampled_event_view),
					 [&current_state](auto __attribute__((unused)) && x) {
					   auto __attribute__((unused)) a = current_state;
					   auto filter = std::get<1>(std::get<0>(x));
					   any_sir_event event = std::get<1>(x);
					   return (filter(event));
					 }),
		     [&counter, &future_states_remained, &future_states_entered](const auto &x) {
		       auto remained_index = std::get<0>(std::get<0>(x));
		       auto tmp = std::get<1>(x);
		       for (auto i : std::ranges::views::iota(0UL, tmp.affected_people.size())) {
			 if (tmp.postconditions[i]) {
			   future_states_entered.potential_states[tmp.affected_people[i]][*(tmp.postconditions[i])] = true;
			   for (auto j :
				  std::ranges::views::iota(0UL, future_states_remained[remained_index].potential_states[tmp.affected_people[i]].size())) {
			     future_states_remained[remained_index].potential_states[tmp.affected_people[i]][j] =
			       future_states_remained[remained_index].potential_states[tmp.affected_people[i]][j] && (!tmp.preconditions[i][j]);
			   }
			 }
		       }
		       ++counter;
		       return;
		     });
    std::cout << counter << std::endl;
    for(const auto& elem: future_states_remained){
      future_states_entered = future_states_entered || elem;
    }
    future_states_entered.time = t;
    print(future_states_entered);
    current_state = future_states_entered;
  }

}

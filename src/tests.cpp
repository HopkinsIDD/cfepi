#include <cfepi/sir_specific.hpp>
#include <cfepi/sample_view.hpp>
#include <iostream>
#include <range/v3/all.hpp>

int main(int argc, char** argv) {
  if (argc != 4) {
    std::cout << "This program takes 3 arguments: population size, number of time steps and seed." << std::endl;
    return 1;
  }
  std::random_device rd;
  size_t seed = static_cast<size_t>(atoi(argv[3]));
  std::default_random_engine random_source_1{ seed++ };
  const person_t population_size = static_cast<person_t>(atoi(argv[1]));

  auto always_true = [](const auto &param __attribute__((unused)) ) { return (true); };
  auto always_false = [](const auto &param __attribute__((unused)) ) { return (false); };
  auto initial_conditions = default_state<epidemic_states>(epidemic_states::S, epidemic_states::I, population_size, 1UL);

  std::vector<std::function<bool(const any_sir_event &)>> filters{always_true, always_false};

  // BEGIN
  auto current_state = initial_conditions;

  auto setups_by_filter = ranges::to<std::vector>(
    ranges::views::transform(filters, [&current_state](auto &x) {
      return (std::make_tuple(current_state, current_state, current_state, x));
    }));
  for(epidemic_time_t t = 0UL; t < static_cast<epidemic_time_t>(atoi(argv[2])); ++t){

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

    std::array<double, 2> event_probabilities = {.5, 2. / static_cast<double>(population_size)};

    const auto single_event_type_run = [ &seed, &t, &setups_by_filter, &random_source_1, &population_size, &current_state, &event_probabilities](const auto event_index){
      random_source_1.seed(seed++);
      auto event_range_generator =
        single_type_event_generator<epidemic_states, event_index>(current_state);
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

      ranges::for_each(filtered_view, [&setups_by_filter, &counter](const auto &x) {
        auto tmp = std::get<1>(x);
        for (auto i : std::ranges::views::iota(0UL, tmp.affected_people.size())) {
          if (tmp.postconditions[i]) {
            auto& local_current_state = std::get<0>(setups_by_filter[std::get<0>(x)]);
  	  if (any_sir_state_check_preconditions{local_current_state}(tmp)) {
  	    auto& states_entered = std::get<1>(setups_by_filter[std::get<0>(x)]);
  	    auto& states_remained = std::get<2>(setups_by_filter[std::get<0>(x)]);

  	    states_entered.potential_states[tmp.affected_people[i]][*(tmp.postconditions[i])] = true;
              std::cout << "Person " << tmp.affected_people[i] << " is entering state " << *(tmp.postconditions[i]) << std::endl;
              for (auto j : std::ranges::views::iota(0UL, std::size(epidemic_states{}))) {
                states_remained.potential_states[tmp.affected_people[i]][j] =
                  states_remained.potential_states[tmp.affected_people[i]][j]
                  && !tmp.preconditions[i][j];
              }
            }
          }
        }
        ++counter;
        return;
      });
      std::cout << counter << std::endl;

      ranges::for_each(setups_by_filter, [& t](auto&x){
        std::get<0>(x) = std::get<1>(x) || std::get<2>(x);
        std::get<0>(x).time = t;
        print(std::get<0>(x));
      });
    };

    single_event_type_run(std::integral_constant<size_t, 0UL>());
    single_event_type_run(std::integral_constant<size_t, 1UL>());
  }

}

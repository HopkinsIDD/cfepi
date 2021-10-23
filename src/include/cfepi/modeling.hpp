#include <cfepi/sir.hpp>


template<typename states_t, typename any_event, size_t nevents>
auto run_simulation(
		    const sir_state<states_t>& initial_conditions,
		    const std::array<double, nevents> event_probabilities,
		    const std::vector<std::function<bool(const any_event &)>> filters,
		    const epidemic_time_t epidemic_duration = 365,
		    size_t simulation_seed = 2
		    ) {
  std::default_random_engine random_source_1{ 1UL };

  // BEGIN
  auto current_state = initial_conditions;

  auto setups_by_filter =
    ranges::to<std::vector>(ranges::views::transform(filters, [&current_state](auto &x) {
      return (std::make_tuple(current_state, current_state, current_state, x));
    }));

  for (epidemic_time_t t = 0UL; t < epidemic_duration; ++t) {

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
                                         &current_state,
                                         &event_probabilities](
                                         const auto event_index, const size_t seed) {
      random_source_1.seed(seed);
      auto event_range_generator =
        single_type_event_generator<states_t, any_event, event_index>(current_state);
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
          any_event event = std::get<1>(x);
          return (filter(event));
        });

      ranges::for_each(filtered_view, [&setups_by_filter, &counter](const auto &x) {
        if (any_state_check_preconditions<any_event, states_t>{ std::get<0>(setups_by_filter[std::get<0>(x)]) }(
              std::get<1>(x))) {
          any_event_apply_entered_states{ std::get<1>(setups_by_filter[std::get<0>(x)]) }(
            std::get<1>(x));
          any_event_apply_left_states{ std::get<2>(setups_by_filter[std::get<0>(x)]) }(
            std::get<1>(x));
        }
	++counter;
      });

      ranges::for_each(setups_by_filter, [&t](auto &x) {
        std::get<0>(x) = std::get<1>(x) || std::get<2>(x);
        std::get<0>(x).time = t;
        print(std::get<0>(x));
      });
    };

    const auto seeded_single_event_type_run = [&single_event_type_run, &simulation_seed](
                                                const auto event_index) {
      ++simulation_seed;
      single_event_type_run(event_index, simulation_seed);
    };

    cfor::constexpr_for<0, 3, 1>(seeded_single_event_type_run);
  }

}

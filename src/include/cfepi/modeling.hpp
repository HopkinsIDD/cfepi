#include <cfepi/sir.hpp>
#include <cfepi/sample_view.hpp>
#include <range/v3/all.hpp>

template<typename states_t, typename any_event> struct filtration_setup {
  sir_state<states_t> current_state;
  sir_state<states_t> states_entered;
  sir_state<states_t> states_remained;
  std::function<bool(const any_event &, std::default_random_engine &)> filter_;
  filtration_setup(const sir_state<states_t> &initial_state,
    const std::function<bool(const any_event &, std::default_random_engine &)>& filter)
    : current_state(initial_state), states_entered(initial_state), states_remained(initial_state),
      filter_(filter) {
    states_entered.reset();
  };
  void reset() {
    current_state.reset();
    current_state = states_entered || states_remained;
    states_entered.reset();
    states_remained = current_state;
  };
};

template<typename states_t, typename any_event>
const auto run_simulation(
		    const sir_state<states_t>& initial_conditions,
		    const std::array<double, std::variant_size_v<any_event>> event_probabilities,
		    const std::vector<std::function<bool(const any_event &, std::default_random_engine&)>> filters,
		    const epidemic_time_t epidemic_duration = 365,
		    size_t simulation_seed = 2
		    ) {
  std::default_random_engine random_source_1{ 1UL };

  // BEGIN
  auto current_state = initial_conditions;

  auto setups_by_filter =
    ranges::to<std::vector>(ranges::views::transform(filters, [&current_state](auto &x) {
      // return (std::make_tuple(current_state, current_state, current_state, x));
      return (filtration_setup{current_state, x});
    }));

  std::vector<sir_state<states_t>> results;
  results.reserve(static_cast<size_t>(epidemic_duration));

  for (epidemic_time_t t = 0UL; t < epidemic_duration; ++t) {

    current_state.reset();
    current_state = std::transform_reduce(
      std::begin(setups_by_filter),
      std::end(setups_by_filter),
      current_state,
      [](const auto &x, const auto &y) { return (x || y); },
      // [](const auto &x) { return (std::get<0>(x)); });
      [](const auto &x) { return (x.current_state); });
    ranges::for_each(setups_by_filter, [](auto &x) {
      // std::get<1>(x).reset();
      // std::get<2>(x) = std::get<0>(x);
      x.reset();
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
					       view_to_filter, [setups_by_filter = std::as_const(setups_by_filter), &random_source_1](const auto &x) {
						 // auto filter = std::get<3>(setups_by_filter[std::get<0>(x)]);
          auto filter = setups_by_filter[std::get<0>(x)].filter_;
          any_event event = std::get<1>(x);
          return (filter(event, random_source_1));
        });

      ranges::for_each(filtered_view, [&setups_by_filter, &counter](const auto &x) {
        // if (any_state_check_preconditions<any_event, states_t>{ std::get<0>(setups_by_filter[std::get<0>(x)]) }(
	// std::get<1>(x))) {
	auto& setup = setups_by_filter[std::get<0>(x)];
        if (any_state_check_preconditions<any_event, states_t>{ setup.current_state }(
              std::get<1>(x))) {
          any_event_apply_entered_states{ setup.states_entered }(
            std::get<1>(x));
          any_event_apply_left_states{ setup.states_remained } (
            std::get<1>(x));
        }
	++counter;
      });

      ranges::for_each(setups_by_filter, [&t](auto &x) {
	x.current_state = x.states_entered || x.states_remained;
	x.current_state.time = t;
	print(x.current_state);
      });
    };

    const auto seeded_single_event_type_run = [&single_event_type_run, &simulation_seed](
                                                const auto event_index) {
      ++simulation_seed;
      single_event_type_run(event_index, simulation_seed);
    };

    cfor::constexpr_for<0, std::variant_size_v<any_event>, 1>(seeded_single_event_type_run);

    ranges::for_each(setups_by_filter, [&results](filtration_setup<states_t, any_event>& x){
      results.push_back(x.current_state);
    });
  }

  return(results);
}

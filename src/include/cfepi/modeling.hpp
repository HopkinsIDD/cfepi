#include <cfepi/sir.hpp>
#include <cfepi/sample_view.hpp>
#include <range/v3/all.hpp>

namespace cfepi {
/*!
 * \class filtration_setup
 * \brief Data structure for storing a single world's worth of data.
 *
 * Includes the current state of the system, pending changes to that state, and filter functions
 * needed to update that state.
 */
template<typename states_t, typename any_event> struct filtration_setup {
  //! \brief The current state of the world
  sir_state<states_t> current_state;
  //! \brief The states that are pending entry since reset
  sir_state<states_t> states_entered;
  //! \brief The states that have not been left since reset
  sir_state<states_t> states_remained;
  //! \brief A filter to use to apply only some events. Returns true if event should be kept.
  std::function<bool(const any_event &, std::default_random_engine &)> event_filter_;
  //! \brief A filter to use to restrict which states are allowed. Applies at each time step to determine if the current state is valid. Returns true if state is ok, and false if this time step should be re-done.
  std::function<bool(const sir_state<states_t>&, std::default_random_engine &)> state_filter_;
  //! \brief A filter to modify the state used to apply certain kinds of interventions.
  std::function<void(sir_state<states_t>&, std::default_random_engine &)> state_modifier_;
  //! \brief Construct from an initial state and a filter. This is the standard constructor
  filtration_setup(const sir_state<states_t> &initial_state,
    const std::function<bool(const any_event &, std::default_random_engine &)> &event_filter,
    const std::function<bool(const sir_state<states_t> &, std::default_random_engine &)> &state_filter,
    const std::function<void(sir_state<states_t> &, std::default_random_engine &)> &state_modifier)
    : current_state(initial_state), states_entered(initial_state), states_remained(initial_state),
      event_filter_(event_filter), state_filter_(state_filter), state_modifier_(state_modifier) {
    states_entered.reset();
  };
  //! \brief Apply pending changes to current state, then clear them
  void apply() {
    current_state = states_entered || states_remained;
    states_entered.reset();
    states_remained = current_state;
  };
  //! \brief Clear pending changes
  void reset() {
    states_entered.reset();
    states_remained = current_state;
  };
};

}// namespace cfepi


namespace cfepi {
//! \defgroup Model_Construction Model Construction
//! @{
/*!
 * \brief Run a counterfactual simulation
 * Run a counterfactual simulation with a different filter for each world.
 * @param initial_conditions An sir_state to use as the state of the population at time 0.
 * @param event_probabilities An array with one element for each event containing the probability of
 * that event.
 * @param filters A vector of filters containing one filter for each scenario.  A filter is a
 * function which takes events and a random number generator and returns true if that event should
 * be kept or false if that event should be discarded.
 * @param epidemic_duration The number of time steps to run the model for (running the model past
 * the last useful day will not dramatically impact runtime)
 * @param simulation_seed Random seed.  Different values will provide different simulations, the
 * same values will provide the same simulations
 * @return A vector of aggregated states, one for each time step.
 */
template<typename states_t, typename any_event>
auto run_simulation(const sir_state<states_t> &initial_conditions,
  const std::array<double, std::variant_size_v<any_event>> event_probabilities,
  const std::vector<
		    std::tuple<
		    std::function<bool(const any_event &, std::default_random_engine &)>,
		    std::function<bool(const sir_state<states_t>&, std::default_random_engine &)>,
		    std::function<void(sir_state<states_t>&, std::default_random_engine &)>
		    >
		   > filters,
  const epidemic_time_t epidemic_duration = 365,
  size_t simulation_seed = 2) {

  std::default_random_engine random_source_1{ 1UL };
  auto current_state = initial_conditions;

  auto setups_by_filter =
    ranges::to<std::vector>(ranges::views::transform(filters, [&current_state](auto &x) {
      return (filtration_setup{current_state, std::get<0>(x), std::get<1>(x), std::get<2>(x)});
    }));

  std::vector results{ { aggregate_state(current_state) } };
  results.clear();
  results.reserve(static_cast<size_t>(epidemic_duration));

  for (epidemic_time_t t = 0UL; t < epidemic_duration; ++t) {

    current_state.reset();
    current_state = std::transform_reduce(
      std::begin(setups_by_filter),
      std::end(setups_by_filter),
      current_state,
      [](const auto &x, const auto &y) { return (x || y); },
      [](const auto &x) { return (x.current_state); });


    const auto single_event_type_run =
      [&t, &setups_by_filter, &random_source_1, &current_state, &event_probabilities](
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

        auto filtered_view = ranges::filter_view(view_to_filter,
          [setups_by_filter = std::as_const(setups_by_filter), &random_source_1](const auto &x) {
            auto filter = setups_by_filter[std::get<0>(x)].event_filter_;
            any_event event = std::get<1>(x);
            return (filter(event, random_source_1));
          });

        ranges::for_each(filtered_view, [&setups_by_filter, &counter](const auto &x) {
          auto &setup = setups_by_filter[std::get<0>(x)];
          if (any_state_check_preconditions<any_event, states_t>{ setup.current_state }(
                std::get<1>(x))) {
            any_event_apply_entered_states{ setup.states_entered }(std::get<1>(x));
            any_event_apply_left_states{ setup.states_remained }(std::get<1>(x));
          }
          ++counter;
        });

      };

    const auto seeded_single_event_type_run = [&single_event_type_run, &simulation_seed](
                                                const auto event_index) {
      ++simulation_seed;
      single_event_type_run(event_index, simulation_seed);
    };

    cfor::constexpr_for<0, std::variant_size_v<any_event>, 1>(seeded_single_event_type_run);

    bool all_states_allowed = std::transform_reduce(
      std::begin(setups_by_filter),
      std::end(setups_by_filter),
      true,
      [](bool x, bool y) { return (x && y); },
      [t,&random_source_1](filtration_setup<states_t, any_event> &x) {
	auto states_next = x.states_entered || x.states_remained;
	states_next.time = t;
	return x.state_filter_(states_next, random_source_1);
      });

    if (all_states_allowed) {
      ranges::for_each(setups_by_filter, [&t](auto &x) {
	x.apply();
	x.current_state.time = t;
      });
      ranges::for_each(setups_by_filter, [&results](filtration_setup<states_t, any_event> &x) {
	results.push_back(aggregate_state(x.current_state));
      });
    } else {
      ranges::for_each(setups_by_filter, [&t](auto &x) {
	x.reset();
      });
      --t;
    }

  }

  return (results);
}
//@}

}// namespace cfepi

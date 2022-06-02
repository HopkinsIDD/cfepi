#include <cfepi/sample_view.hpp>
#include <cfepi/sir.hpp>
#include <range/v3/all.hpp>

#ifndef __MODELING_H_
#define __MODELING_H_

namespace cfepi {

/*!
 * \class filtration_setup
 * \brief Data structure for storing a single world's worth of data.
 *
 * Includes the current state of the system, pending changes to that state, and filter functions
 * needed to update that state.
 */
template<typename states_t, typename any_event> struct filtration_setup
{
  using filtration_tuple =
    std::tuple<std::function<bool(const any_event &, const sir_state<states_t> &, std::default_random_engine &)>,
      std::function<
        bool(const filtration_setup<states_t, any_event> &, const sir_state<states_t> &, std::default_random_engine &)>,
      std::function<void(sir_state<states_t> &, std::default_random_engine &)>>;

  //! \brief The current state of the world
  sir_state<states_t> current_state;
  //! \brief The states that are pending entry since reset
  sir_state<states_t> states_entered;
  //! \brief The states that have not been left since reset
  sir_state<states_t> states_remained;
  //! \brief A filter to use to apply only some events. Returns true if event should be kept.
  std::function<bool(const any_event &, const sir_state<states_t> &, std::default_random_engine &)> event_filter_;
  //! \brief A filter to use to restrict which states are allowed. Applies at each time step to
  //! determine if the current state is valid. Returns true if state is ok, and false if this time
  //! step should be re-done.
  std::function<
    bool(const filtration_setup<states_t, any_event> &, const sir_state<states_t> &, std::default_random_engine &)>
    state_filter_;
  //! \brief A filter to modify the state used to apply certain kinds of interventions.
  std::function<void(sir_state<states_t> &, std::default_random_engine &)> state_modifier_;
  //! \brief Construct from an initial state and a filter. This is the standard constructor
  filtration_setup(const sir_state<states_t> &initial_state,
    const std::function<bool(const any_event &, const sir_state<states_t> &, std::default_random_engine &)>
      &event_filter,
    const std::function<bool(const filtration_setup<states_t, any_event> &,
      const sir_state<states_t> &,
      std::default_random_engine &)> &state_filter,
    const std::function<void(sir_state<states_t> &, std::default_random_engine &)> &state_modifier)
    : current_state(initial_state), states_entered(initial_state), states_remained(initial_state),
      event_filter_(event_filter), state_filter_(state_filter), state_modifier_(state_modifier)
  {
    states_entered.reset();
  };
  filtration_setup(const sir_state<states_t> &initial_state, const filtration_tuple &filters )
    : current_state(initial_state), states_entered(initial_state), states_remained(initial_state),
      event_filter_(std::get<0>(filters)), state_filter_(std::get<1>(filters)), state_modifier_(std::get<2>(filters))
  {
    states_entered.reset();
  };
  //! \brief Apply pending changes to current state, then clear them
  void apply()
  {
    current_state = states_entered || states_remained;
    states_entered.reset();
    states_remained = current_state;
  };
  //! \brief Clear pending changes
  void reset()
  {
    states_entered.reset();
    states_remained = current_state;
  };
};

}// namespace cfepi


namespace cfepi {

template<typename states_t, typename any_event_type, typename any_event>
auto single_event_type_run(auto &all_event_types,
  auto &setups_by_filter,
  auto &random_source_1,
  auto &current_state,
  auto &event_probabilities,
  const auto event_index,
  const size_t seed)
{
  random_source_1.seed(seed);
  auto event_range_generator = single_type_event_generator<std::variant_alternative_t<event_index, any_event_type>>(
    std::get<event_index>(all_event_types), current_state);

  static_assert(std::ranges::input_range<decltype(event_range_generator.cartesian_range())>);
  if constexpr (!std::ranges::input_range<decltype(event_range_generator.cartesian_range())>) {
    throw "This shouldn't happen";
  }
  auto this_event_range = event_range_generator.event_range();

  auto sampled_event_view =
    this_event_range | probability::views::sample(event_probabilities[event_index], random_source_1);

  size_t counter = 0UL;
  auto setup_view = std::ranges::views::iota(0UL, setups_by_filter.size());
  auto view_to_filter = ranges::views::cartesian_product(setup_view, sampled_event_view);

  auto filtered_view = ranges::filter_view(
    view_to_filter, [setups_by_filter = std::as_const(setups_by_filter), &random_source_1](const auto &x) {
      const auto &filter = setups_by_filter[std::get<0>(x)].event_filter_;
      const auto &state = setups_by_filter[std::get<0>(x)].current_state;
      const auto &event = std::get<1>(x);
      return (filter(event, state, random_source_1));
    });

  for(const auto x : filtered_view) {
    auto &setup = setups_by_filter[std::get<0>(x)];
    if (any_state_check_preconditions<any_event_type, states_t>{ setup.current_state }(std::get<1>(x))) {
      any_event_apply_entered_states{ setup.states_entered }(std::get<1>(x));
      any_event_apply_left_states{ setup.states_remained }(std::get<1>(x));
    }
    ++counter;
  }
};

template<typename states_t, typename any_event_type, typename any_event>
auto single_time_run(auto &results,
  auto &setups_by_filter,
  auto &all_event_types,
  auto &t,
  auto &random_source_1,
  auto &event_probabilities,
  auto &simulation_seed,
  auto &resets)
{

  if (std::begin(setups_by_filter) == std::end(setups_by_filter)){
    throw "There should be at least one setup\n";
  }
  auto first_setup = (*std::begin(setups_by_filter)).current_state;
  const auto current_state = std::transform_reduce(
    std::begin(setups_by_filter)+1,
    std::end(setups_by_filter),
    first_setup,
    [](const auto &x, const auto &y) { return (x || y); },
    [](const auto &x) { return (x.current_state); });

  bool still_working = true;
  while( still_working ) {
    still_working = single_reset_run<states_t, any_event_type, any_event>(
      setups_by_filter,
      current_state,
      all_event_types,
      t,
      random_source_1,
      event_probabilities,
      simulation_seed);
    ++resets;
  }
  --resets;

  results.push_back(ranges::to<std::vector>(
    ranges::views::transform(setups_by_filter, [](const auto &x) { return (aggregate_state(x.current_state)); })));
}

template<typename states_t, typename any_event_type, typename any_event>
auto single_reset_run(
  auto &setups_by_filter,
  const auto &current_state,
  auto &all_event_types,
  auto &t,
  auto &random_source_1,
  auto &event_probabilities,
  auto &simulation_seed)
{

  const auto seeded_single_event_type_run =
    [&all_event_types, &t, &setups_by_filter, &random_source_1, &current_state, &event_probabilities, &simulation_seed](
      const auto event_index) {
      simulation_seed = random_source_1();
      // ++simulation_seed;
      single_event_type_run<states_t, any_event_type, any_event>(all_event_types,
        setups_by_filter,
        random_source_1,
        current_state,
        event_probabilities,
        event_index,
        simulation_seed);
    };

  cfor::constexpr_for<0, std::variant_size_v<any_event_type>, 1>(seeded_single_event_type_run);

  // Factor out into function
  const auto update_lambda = [t, &random_source_1](auto &x) {
    auto rc{ x.states_entered || x.states_remained };
    rc.time = t;
    x.state_modifier_(rc, random_source_1);
    return (rc);
  };

  auto states_next = ranges::to<std::vector>(ranges::views::transform(setups_by_filter, update_lambda));

  bool all_states_allowed = std::transform_reduce(
    std::begin(setups_by_filter),
    std::end(setups_by_filter),
    std::begin(states_next),
    true,
    [](bool x, bool y) { return (x && y); },
    [t, &random_source_1](filtration_setup<states_t, any_event> &setup, sir_state<states_t> new_state) {
      return setup.state_filter_(setup, new_state, random_source_1);
    });

  if (all_states_allowed) {
    for (auto i : std::ranges::views::iota(0UL, std::size(states_next))) {
      setups_by_filter[i].current_state = states_next[i];
      setups_by_filter[i].reset();
    }
    return false;
  } else {
    ranges::for_each(setups_by_filter, [&t](auto &x) { x.reset(); });
    return(true);
  }
}

template<typename states_t, typename any_event>
using filtration_tuple = filtration_setup<states_t, any_event>::filtration_tuple;

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
template<typename states_t, typename any_event_type, typename any_event>
auto run_simulation(auto all_event_types,
  const sir_state<states_t> &initial_conditions,
  const std::array<double, std::variant_size_v<any_event_type>> event_probabilities,
  const std::vector<filtration_tuple<states_t, any_event>> &filters,
  const epidemic_time_t epidemic_duration = 365,
  size_t simulation_seed = 2)
{
  size_t resets = 0;

  std::default_random_engine random_source_1{ simulation_seed };

  std::vector<filtration_setup<states_t, any_event>> setups_by_filter{};
  for(auto filter : filters) {
    setups_by_filter.push_back(filtration_setup<states_t, any_event>(initial_conditions, filter));
  }

  std::vector<aggregated_sir_state<states_t>> first_result{};
  for(auto setup : setups_by_filter){
    first_result.push_back(aggregate_state(setup.current_state));
  }

  std::vector<std::vector<aggregated_sir_state<states_t>>> results{first_result};
  results.reserve(static_cast<size_t>(epidemic_duration + 1));

  results.push_back(first_result);

  for (epidemic_time_t t = 0UL; t < epidemic_duration; ++t) {
    single_time_run<states_t, any_event_type, any_event>(results,
      setups_by_filter,
      all_event_types,
      t,
      random_source_1,
      event_probabilities,
      simulation_seed,
      resets);
  }

  std::cout << "Ran with " << resets << " resets\n";

  return (results);
}
//@}

}// namespace cfepi

#endif

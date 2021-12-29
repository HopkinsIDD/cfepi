#include <cfepi/modeling.hpp>
#include <cfepi/sample_view.hpp>
#include <iostream>
#include <range/v3/all.hpp>

namespace detail {
struct epidemic_states {
public:
  enum state { S, I, R, V, n_compartments };
  constexpr static auto size() { return (static_cast<size_t>(n_compartments)); }
};

struct infection_event : public cfepi::interaction_event<epidemic_states> {
  constexpr infection_event(cfepi::person_t p1,
    cfepi::person_t p2,
    cfepi::epidemic_time_t _time) noexcept
    : cfepi::interaction_event<epidemic_states>(p1,
      p2,
      _time,
      { std::bitset<std::size(epidemic_states{})>{
        (1 << epidemic_states::S) +
        (1 << epidemic_states::V)
      } },
      { std::bitset<std::size(epidemic_states{})>{ 1 << epidemic_states::I } },
      epidemic_states::I){};
  constexpr infection_event() noexcept : infection_event(0UL, 0UL, -1){};
};

struct recovery_event : public cfepi::transition_event<epidemic_states> {
  constexpr recovery_event(cfepi::person_t p1, cfepi::epidemic_time_t _time) noexcept
    : cfepi::transition_event<epidemic_states>(p1,
      _time,
      { std::bitset<std::size(epidemic_states{})>{ 1 << epidemic_states::I } },
      epidemic_states::R){};
  constexpr recovery_event() noexcept : recovery_event(0UL, -1){};
};

typedef std::variant<infection_event, recovery_event> any_event;

// template<> struct event_by_index<any_event, 1> : recovery_event {};
// template<> struct event_by_index<any_event, 0> : infection_event {};

}// namespace detail

using namespace detail;

int main(int argc, char **argv) {

  const cfepi::person_t population_size = static_cast<cfepi::person_t>(273613);
  double gamma = 1 / 2.25;
  if (argc > 2) {
    gamma = 1.0 / atof(argv[2]);
  }
  double beta = 1.05 * gamma / static_cast<double>(population_size);
  if (argc > 1) {
    beta = atof(argv[1]) * gamma / static_cast<double>(population_size);
  }
  cfepi::epidemic_time_t full_simulation_time{100};
  if (argc > 3) {
    std::cout << argc << "\n";
    full_simulation_time = static_cast<cfepi::epidemic_time_t>(atof(argv[3]));
  }
  size_t seed{1};
  if (argc > 4) {
    seed = static_cast<size_t>(atoi(argv[4]));
  }
  const cfepi::epidemic_time_t epidemic_time = full_simulation_time;
  std::random_device rd;
  std::default_random_engine random_source_1{ seed++ };

  auto vaccination_reduction = [](
				  const auto &event,
				  const auto &state,
				  std::default_random_engine &rng
				  ) {
    std::uniform_real_distribution<> dist(0.0, 1.0);
    if (std::holds_alternative<infection_event>(event)) {
      if (dist(rng) < .7) {
	cfepi::person_t this_person{std::visit(cfepi::any_sir_event_get_affected_person<epidemic_states>{0},event)};
	return(!state.potential_states[this_person][epidemic_states::V]);
      }
    }
    return (true);
  };
  constexpr cfepi::epidemic_time_t simulation_length{57};
  std::array<cfepi::person_t, static_cast<size_t>(simulation_length + 1)> counts_to_filter_to{
1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 6, 1, 1, 3, 3, 4, 8, 2, 4, 2, 7, 3, 9, 4, 5, 7, 8, 4, 7, 7, 6, 7, 9, 7, 9, 12, 7, 13, 15, 8, 21, 17, 11, 11, 14, 11, 3, 4, 14, 9, 7, 8
  };
  const auto filter_by_infected = [&counts_to_filter_to](
                              const cfepi::filtration_setup<epidemic_states, any_event> &setup,
                              const cfepi::sir_state<epidemic_states> &new_state,
                              std::default_random_engine &rng __attribute__((unused))) {
    std::size_t this_time = static_cast<size_t>(new_state.time);
    if (this_time <= simulation_length) {
      auto incidence_counts = cfepi::aggregate_state(setup.states_entered).potential_state_counts[1 << epidemic_states::I];
      if(incidence_counts < counts_to_filter_to[this_time]) {
	std::cout << "low ";
      } else {
	std::cout << "high ";
      }
      std::cout << incidence_counts << " modeled vs " << counts_to_filter_to[this_time] << " expected\n";
      return (incidence_counts == counts_to_filter_to[this_time]);
    }
    return (true);
  };
  auto do_vaccination = [](double vaccination_percentage) {
    return ([vaccination_percentage](cfepi::sir_state<epidemic_states> &state, std::default_random_engine &rng) {
      std::cout << "Time is " << state.time << "\n";
      if (state.time > 0) {
	return;
      }
      std::cout << "VACCINATING at time " << state.time << "!\n";
      std::uniform_real_distribution<> dist(0.0, 1.0);
      for(auto& this_state : state.potential_states) {
	if (dist(rng) < vaccination_percentage) {
	  this_state.set(epidemic_states::V, true);
	  this_state.set(epidemic_states::S, false);
	}
      }
      return;
    });
  };
  auto always_true_state = [](const auto &setup __attribute__((unused)),
			      const auto &new_state __attribute__((unused)),
                             std::default_random_engine &rng
                             __attribute__((unused))) { return (true); };

  std::cout << "gamma is " << gamma << "\nbeta is " << beta << "\n";
  auto initial_conditions = cfepi::default_state<epidemic_states>(
    epidemic_states::S, epidemic_states::I, population_size, 1UL);

  auto simulation = cfepi::run_simulation<epidemic_states, any_event>(initial_conditions,
    std::array<double, 2>({ beta, gamma }),
    { std::make_tuple(vaccination_reduction, filter_by_infected, do_vaccination(.24)),
      std::make_tuple(vaccination_reduction, always_true_state, do_vaccination(.5)) },
    epidemic_time,
    seed);

  for (auto state_group : simulation) {
    size_t state_group_index{ 0 };
    for (auto state : state_group) {
      cfepi::print(state, "group " + std::to_string(state_group_index) + " : ");
      ++state_group_index;
    }
  }
}

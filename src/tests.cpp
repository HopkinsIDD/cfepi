#include <cfepi/modeling.hpp>
#include <cfepi/sample_view.hpp>
#include <iostream>
#include <range/v3/all.hpp>

namespace detail {
struct sir_epidemic_states {
public:
  enum state { S, I, R, n_compartments };
  constexpr static auto size() { return (static_cast<size_t>(n_compartments)); }
};

struct infection_event : public cfepi::interaction_event<sir_epidemic_states> {
  constexpr infection_event(cfepi::person_t p1,
    cfepi::person_t p2,
    cfepi::epidemic_time_t _time) noexcept
    : cfepi::interaction_event<sir_epidemic_states>(p1,
      p2,
      _time,
      { std::bitset<std::size(sir_epidemic_states{})>{ 1 << sir_epidemic_states::S } },
      { std::bitset<std::size(sir_epidemic_states{})>{ 1 << sir_epidemic_states::I } },
      sir_epidemic_states::I){};
  constexpr infection_event() noexcept : infection_event(0UL, 0UL, -1){};
};

struct recovery_event : public cfepi::transition_event<sir_epidemic_states> {
  constexpr recovery_event(cfepi::person_t p1, cfepi::epidemic_time_t _time) noexcept
    : cfepi::transition_event<sir_epidemic_states>(p1,
      _time,
      { std::bitset<std::size(sir_epidemic_states{})>{ 1 << sir_epidemic_states::I } },
      sir_epidemic_states::R){};
  constexpr recovery_event() noexcept : recovery_event(0UL, -1){};
};

typedef std::variant<infection_event, recovery_event> any_sir_event;

// template<> struct event_by_index<any_sir_event, 1> : recovery_event {};
// template<> struct event_by_index<any_sir_event, 0> : infection_event {};

}// namespace detail

using namespace detail;

int main(int argc, char **argv) {

  if (argc != 4) {
    std::cout << "This program takes 3 arguments: population size, number of time steps and seed."
              << std::endl;
    return 1;
  }
  size_t seed = static_cast<size_t>(atoi(argv[3]));
  const cfepi::person_t population_size = static_cast<cfepi::person_t>(atoi(argv[1]));
  const cfepi::epidemic_time_t epidemic_time = static_cast<cfepi::epidemic_time_t>(atof(argv[2]));
  std::random_device rd;
  std::default_random_engine random_source_1{ seed++ };

  auto always_true_event = [](const auto &param __attribute__((unused)), const auto &state __attribute__((unused)),
                       std::default_random_engine &rng __attribute__((unused))) { return (true); };
  auto always_true_state = [](const auto &first_param __attribute__((unused)),
			      const auto &second_param __attribute__((unused)),
                       std::default_random_engine &rng __attribute__((unused))) { return (true); };
  /*auto always_false = [](const auto &param __attribute__((unused)),
                        std::default_random_engine &rng
                        __attribute__((unused))) { return (false); };
  */
  auto sometimes_true = [](const any_sir_event &event, const cfepi::sir_state<sir_epidemic_states> &state __attribute__((unused)), std::default_random_engine &rng) {
    std::uniform_real_distribution<> dist(0.0, 1.0);
    if (std::holds_alternative<infection_event>(event)) {
      if (dist(rng) < .1) { return (false); }
    }
    return (true);
  };
  auto do_nothing = [](auto &param __attribute__((unused)),
                       std::default_random_engine &rng __attribute__((unused))) { return; };

  double gamma = 1 / 2.25;
  double beta = 1.75 * gamma / static_cast<double>(population_size);
  std::cout << "gamma is " << gamma << "\nbeta is " << beta << "\n";
  auto initial_conditions = cfepi::default_state<sir_epidemic_states>(
    sir_epidemic_states::S, sir_epidemic_states::I, population_size, 1UL);

  auto simulation = cfepi::run_simulation<sir_epidemic_states, any_sir_event>(initial_conditions,
    std::array<double, 2>({ beta, gamma }),
    {
      std::make_tuple(always_true_event, always_true_state, do_nothing),
      std::make_tuple(sometimes_true, always_true_state, do_nothing)
      // std::make_tuple(always_false, always_true, do_nothing),
    },
    epidemic_time,
    seed);

  for (auto state_group : simulation) {
    size_t state_group_index{0};
    for (auto state : state_group) {
      cfepi::print(state, "group " + std::to_string(state_group_index) + " : ");
      ++state_group_index;
    }
  }
}

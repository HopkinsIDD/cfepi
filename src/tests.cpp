#include <cfepi/modeling.hpp>
#include <cfepi/sample_view.hpp>
#include <iostream>
#include <range/v3/all.hpp>

struct sir_epidemic_states {
public:
  enum state {S,I,R,n_compartments};
  constexpr auto size() const {return (static_cast<size_t>(n_compartments));}
};

struct infection_event : public interaction_event<sir_epidemic_states> {
  constexpr infection_event(person_t p1, person_t p2, epidemic_time_t _time) noexcept
    : interaction_event<sir_epidemic_states>(p1,
      p2,
      _time,
      { std::bitset<std::size(sir_epidemic_states{})>{
        1 << sir_epidemic_states::S} },
      { std::bitset<std::size(sir_epidemic_states{})>{
        1 << sir_epidemic_states::I} },
      sir_epidemic_states::I){};
  constexpr infection_event() noexcept : infection_event(0UL, 0UL, -1) {};
};

struct recovery_event : public transition_event<sir_epidemic_states> {
  constexpr recovery_event(person_t p1, epidemic_time_t _time) noexcept
    : transition_event<sir_epidemic_states>(p1,
      _time,
      { std::bitset<std::size(sir_epidemic_states{})>{
        1 << sir_epidemic_states::I} },
      sir_epidemic_states::R){};
  constexpr recovery_event() noexcept : recovery_event(0UL, -1) {};
};

typedef std::variant<infection_event, recovery_event> any_sir_event;

template<> struct event_by_index<any_sir_event, 1> : recovery_event {};
template<> struct event_by_index<any_sir_event, 0> : infection_event {};

int main(int argc, char** argv) {

  if (argc != 4) {
    std::cout << "This program takes 3 arguments: population size, number of time steps and seed." << std::endl;
    return 1;
  }
  std::random_device rd;
  size_t seed = static_cast<size_t>(atoi(argv[3]));
  std::default_random_engine random_source_1{ seed++ };
  const person_t population_size = static_cast<person_t>(atoi(argv[1]));

  auto always_true = [](const auto &param __attribute__((unused)),
                       std::default_random_engine& rng __attribute__((unused))) { return (true); };
  auto always_false = [](const auto &param __attribute__((unused)),
                        std::default_random_engine& rng __attribute__((unused))) { return (false); };
  auto sometimes_true = [](const any_sir_event &event, std::default_random_engine& rng ) {
    std::uniform_real_distribution<> dist(0.0,1.0);
    if (std::holds_alternative<infection_event>(event)) {
      if (dist(rng) < .1) {
	return (false);
      }
    }
    return (true);
  };

  double gamma = 1 / 2.25;
  double beta = 1.75 * gamma / static_cast<double>(population_size);
  std::cout << "gamma is " << gamma << "\nbeta is " << beta << "\n";
  auto initial_conditions = default_state<sir_epidemic_states>(
    sir_epidemic_states::S, sir_epidemic_states::I, population_size, 1UL);

  std::vector<std::function<bool(const any_sir_event &, std::default_random_engine&)>> filters{always_true, sometimes_true, always_false};

  run_simulation<sir_epidemic_states, any_sir_event>(
							   initial_conditions,
							   std::array<double, 2>({ beta, gamma }),
							   filters
							   );

}

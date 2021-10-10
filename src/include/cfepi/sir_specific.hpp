#include <cfepi/sir.hpp>
/*******************************************************************************
 * SIR Helper Type Definitions                                                 *
 *******************************************************************************/

struct epidemic_states{
public:
  enum state { S, I, R, n_SIR_compartments };
  constexpr auto size() const {
    return(static_cast<size_t>(n_SIR_compartments));
  }
};


size_t global_event_counter = 0;

auto sample_sir_state = default_state<epidemic_states>(epidemic_states::S, epidemic_states::I, 1UL, 0UL);

/*
struct infection_event : public sir_event<epidemic_states, 2> {
  using sir_event<epidemic_states, 2>::time;
  using sir_event<epidemic_states, 2>::affected_people;
  using sir_event<epidemic_states, 2>::preconditions;
  using sir_event<epidemic_states, 2>::postconditions;
  constexpr infection_event() noexcept {
    preconditions = { std::array<bool, 3>({ true, false, false }),
      std::array<bool, 3>({ false, true, false }) };
    postconditions[0] = epidemic_states::I;
  }
  infection_event(const infection_event &) = default;
  infection_event(person_t p1, person_t p2, epidemic_time_t _time) {
    time = _time;
    affected_people = { p1, p2 };
    preconditions = { std::array<bool, 3>({ true, false, false }),
      std::array<bool, 3>({ false, true, false }) };
    postconditions[0] = epidemic_states::I;
  }
};
*/
struct infection_event : public interaction_event<epidemic_states> {
  constexpr infection_event() noexcept : interaction_event<epidemic_states>({true, false, false}, {false, true, false}, epidemic_states::I) {};
  constexpr infection_event(person_t p1, person_t p2, epidemic_time_t _time) noexcept : interaction_event<epidemic_states>(p1, p2, _time, {true, false, false}, {false, true, false}, epidemic_states::I) {};
};

struct recovery_event : public transition_event<epidemic_states> {
  constexpr recovery_event() noexcept : transition_event<epidemic_states>({false, true, false}, epidemic_states::R) {};
  constexpr recovery_event(person_t p1, epidemic_time_t _time) noexcept : transition_event<epidemic_states>(p1, _time, {false, true, false}, epidemic_states::R) {};
};

typedef std::variant<recovery_event, infection_event> any_sir_event;

template<> struct sir_event_true_preconditions<0, 0> { constexpr static auto value = { epidemic_states::I }; };

template<> struct sir_event_true_preconditions<1, 0> { constexpr static auto value = { epidemic_states::S }; };

template<> struct sir_event_true_preconditions<1, 1> { constexpr static auto value = { epidemic_states::I }; };

template<> struct sir_event_by_index<0> : recovery_event {};

template<> struct sir_event_by_index<1> : infection_event {};

/*
 * Printing helpers
 */

void print(const any_sir_event &event, std::string prefix = "") {
  std::visit(any_sir_event_print{ prefix }, event);
}

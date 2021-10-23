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

struct infection_event : public interaction_event<epidemic_states> {
  constexpr infection_event() noexcept : interaction_event<epidemic_states>({true, false, false}, {false, true, false}, epidemic_states::I) {};
  constexpr infection_event(person_t p1, person_t p2, epidemic_time_t _time) noexcept : interaction_event<epidemic_states>(p1, p2, _time, {true, false, false}, {false, true, false}, epidemic_states::I) {};
};

struct recovery_event : public transition_event<epidemic_states> {
  constexpr recovery_event() noexcept : transition_event<epidemic_states>({false, true, false}, epidemic_states::R) {};
  constexpr recovery_event(person_t p1, epidemic_time_t _time) noexcept : transition_event<epidemic_states>(p1, _time, {false, true, false}, epidemic_states::R) {};
};

typedef std::variant<recovery_event, infection_event> any_sir_event;

template<> struct event_true_preconditions<any_sir_event, 0, 0> { constexpr static auto value = { epidemic_states::I }; };

template<> struct event_true_preconditions<any_sir_event, 1, 0> { constexpr static auto value = { epidemic_states::S }; };

template<> struct event_true_preconditions<any_sir_event, 1, 1> { constexpr static auto value = { epidemic_states::I }; };

template<> struct event_by_index<any_sir_event, 0> : recovery_event {};

template<> struct event_by_index<any_sir_event, 1> : infection_event {};

/*
 * Printing helpers
 */

void print(const any_sir_event &event, std::string prefix = "") {
  std::visit(any_sir_event_print{ prefix }, event);
}

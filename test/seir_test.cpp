#include <catch2/catch.hpp>
#include <cfepi/sample_view.hpp>
#include <range/v3/all.hpp>

#include <cfepi/sir.hpp>
#include <cfepi/modeling.hpp>

struct seir_epidemic_states {
public:
  enum state { S, E, I, R, n_compartments };
  constexpr auto size() const { return (static_cast<size_t>(n_compartments)); }
};

struct exposure_event : public interaction_event<seir_epidemic_states> {
  constexpr exposure_event(person_t p1, person_t p2, epidemic_time_t _time) noexcept
    : interaction_event<seir_epidemic_states>(p1,
      p2,
      _time,
      { std::bitset<std::size(seir_epidemic_states{})>{
        1 << seir_epidemic_states::S} },
      { std::bitset<std::size(seir_epidemic_states{})>{
        1 << seir_epidemic_states::I} },
      seir_epidemic_states::E){};
  constexpr exposure_event() noexcept : exposure_event(0UL, 0UL, -1) {};
};

struct recovery_event : public transition_event<seir_epidemic_states> {
  constexpr recovery_event(person_t p1, epidemic_time_t _time) noexcept
    : transition_event<seir_epidemic_states>(p1,
      _time,
      { std::bitset<std::size(seir_epidemic_states{})>{
        1 << seir_epidemic_states::I} },
      seir_epidemic_states::R){};
  constexpr recovery_event() noexcept : recovery_event(0UL, -1) {};
};

struct infection_event : public transition_event<seir_epidemic_states> {
  constexpr infection_event(person_t p1, epidemic_time_t _time) noexcept
    : transition_event<seir_epidemic_states>(p1,
      _time,
      { std::bitset<std::size(seir_epidemic_states{})>{
        1 << seir_epidemic_states::E} },
      seir_epidemic_states::I){};
  constexpr infection_event() noexcept : infection_event(0UL, -1) {};
};

typedef std::variant<recovery_event, infection_event, exposure_event> any_seir_event;

template<> struct event_true_preconditions<any_seir_event, 0, 0> {
  constexpr static auto value = { seir_epidemic_states::I };
};
template<> struct event_true_preconditions<any_seir_event, 1, 0> {
  constexpr static auto value = { seir_epidemic_states::E };
};
template<> struct event_true_preconditions<any_seir_event, 2, 0> {
  constexpr static auto value = { seir_epidemic_states::S };
};
template<> struct event_true_preconditions<any_seir_event, 2, 1> {
  constexpr static auto value = { seir_epidemic_states::I };
};

template<> struct event_by_index<any_seir_event, 0> : recovery_event {};
template<> struct event_by_index<any_seir_event, 1> : infection_event {};
template<> struct event_by_index<any_seir_event, 2> : exposure_event {};

TEST_CASE("SEIR_2 model works modularly", "[sir_generator]") {
  person_t population_size = 10000;
  auto initial_conditions = default_state<seir_epidemic_states>(
    seir_epidemic_states::S, seir_epidemic_states::I, population_size, 1UL);
  auto always_true = [](const auto &param __attribute__((unused)), std::default_random_engine& rng __attribute__((unused))) { return (true); };
  run_simulation<seir_epidemic_states, any_seir_event>(initial_conditions,
    std::array<double, 3>({ .1, .8, 2. / static_cast<double>(population_size) }),
    { always_true, always_true });
}

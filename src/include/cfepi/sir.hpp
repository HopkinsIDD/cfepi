#include <chrono>// std::chrono::seconds
#include <iostream>// std::cout, std::endl
#include <thread>// std::this_thread::sleep_for
// this_thread::yield example
#include <atomic>// std::atomic
#include <functional>
#include <mutex>
#include <queue>
#include <typeinfo>
#include <variant>
#include <vector>
#include <span>
#include <bitset>

#include <cor3ntin/rangesnext/to.hpp>
#include <cor3ntin/rangesnext/product.hpp>
#include <cor3ntin/rangesnext/enumerate.hpp>
// #include <range/v3/all.hpp>
// #include <stxxl/vector>

#ifndef __SIR_H_
#define __SIR_H_

/*******************************************************************************
 * Generic functions                                                           *
 *******************************************************************************/

template<typename T, size_t i> using repeat = T;

typedef float epidemic_time_t;
typedef size_t person_t;


/*******************************************************************************
 * SIR Helper Type Definitions                                                 *
 *******************************************************************************/

template<typename enum_type>
concept is_sized_enum = requires(enum_type e) {
  // { e.state };
  { std::size(e) } -> std::unsigned_integral;
} &&
std::default_initializable<enum_type> &&
std::is_enum<typename enum_type::state>::value;

/*******************************************************************************
 * SIR State Definition                                                        *
 *******************************************************************************/

/*
 * @name State
 * @description The state of the model.
 */

template<typename states_t>
requires is_sized_enum<states_t>
struct sir_state {
  person_t population_size = 0;
  std::vector<std::bitset<std::size(states_t{})> > potential_states;
  epidemic_time_t time = -1;
  std::string prefix;
  sir_state() noexcept = default;
  sir_state(person_t _population_size) noexcept : population_size(_population_size) {
    potential_states.resize(population_size);
  }
  sir_state operator||(const sir_state &other) const {
    sir_state rc{*this};
    if (rc.potential_states.size() != other.potential_states.size()) {
      throw "Cannot compare sir_states with different sizes";
    }

    std::ranges::transform(
			   this->potential_states,
			   other.potential_states,
			   std::begin(rc.potential_states),
			   std::bit_or<>(),
			   {},
			   {}
			   );
    return (rc);
  }

  void reset() {
    for (auto &i : potential_states) {
      //for (auto &j : i) { j = false; }
      i.reset();
    }
  }
};

template<typename states_t>
sir_state<states_t> default_state(
				  const typename states_t::state base_state,
				  const typename states_t::state infected_state,
				  const person_t population_size = 10,
				  const person_t initial_infected = 1
				  ) {
  sir_state<states_t> rc{population_size};
  auto base_range = std::ranges::views::iota(population_size * 0, population_size);
  for (auto elem : base_range | std::ranges::views::take(initial_infected)) {
    rc.potential_states[elem][infected_state] = true;
  }
  for (auto elem : base_range | std::ranges::views::drop(initial_infected)) {
    rc.potential_states[elem][base_state] = true;
  }
  return (rc);
}


/*******************************************************************************
 * SIR Event Definition                                                        *
 *******************************************************************************/

/*
 * @name SIR Event
 * @description An event that transitions the model between states
 */

template<typename states_t, size_t size> struct sir_event {
  epidemic_time_t time = -1;
  std::array<person_t, size> affected_people = {};
  std::array<std::bitset<std::size(states_t{})>, size> preconditions = {};
  std::array<std::optional<typename states_t::state>, size> postconditions;
  sir_event() noexcept = default;
  sir_event(const sir_event &) = default;
};

template<typename states_t>
struct interaction_event : public sir_event<states_t, 2> {
  using sir_event<states_t, 2>::time;
  using sir_event<states_t, 2>::affected_people;
  using sir_event<states_t, 2>::preconditions;
  using sir_event<states_t, 2>::postconditions;
  interaction_event(const interaction_event &) = default;
  constexpr interaction_event(
		  person_t p1,
		  person_t p2,
		  epidemic_time_t _time,
		  const std::bitset<std::size(states_t{})> preconditions_for_first_person,
		  const std::bitset<std::size(states_t{})> preconditions_for_second_person,
		  const typename states_t::state result_state
		  ) {
    time = _time;
    affected_people = { p1, p2 };
    preconditions = { preconditions_for_first_person, preconditions_for_second_person };
    postconditions[0] = result_state;
  }
  constexpr interaction_event(
			    const std::array<bool, std::size(states_t{})> preconditions_for_first_person,
			    const std::array<bool, std::size(states_t{})> preconditions_for_second_person,
			    const typename states_t::state result_state
			      ) noexcept : interaction_event(0UL,0UL,-1,preconditions_for_first_person, preconditions_for_second_person, result_state) {};
};

template<typename states_t>
struct transition_event : public sir_event<states_t, 1> {
  using sir_event<states_t, 1>::time;
  using sir_event<states_t, 1>::affected_people;
  using sir_event<states_t, 1>::preconditions;
  using sir_event<states_t, 1>::postconditions;
  transition_event(const transition_event &) = default;
  constexpr transition_event(person_t p1,
    epidemic_time_t _time,
    const std::bitset<std::size(states_t{})> _preconditions,
    const states_t::state result_state) {
    time = _time;
    affected_people = { p1 };
    preconditions = { _preconditions };
    postconditions[0] = result_state;
  }
  constexpr transition_event(
			    const std::array<bool, std::size(states_t{})> _preconditions,
			    const typename states_t::state result_state
			     ) noexcept : transition_event(0,-1,_preconditions,result_state) {}
};

// Users will need to write these :
template<typename any_event, size_t event_index> struct event_by_index;
template<typename any_event, size_t index, size_t permutation_index> struct event_true_preconditions;

template<size_t N, typename = std::make_index_sequence<N>>
struct sir_event_constructor;

/*
template<typename any_sir_event, size_t N, size_t... indices>
struct sir_event_constructor<N, std::index_sequence<indices...>> {
  any_sir_event construct_sir_event(size_t event_index) {
    any_sir_event rc;
    bool any_possible = (false || ... || (event_index == indices));
    if (!any_possible) {
      std::cerr << "Printing index was out of bounds" << std::endl;
      return rc;
    }
    ((rc = (event_index == indices) ? sir_event_by_index<indices>() : rc), ...);
    return rc;
  }
};
*/


/*******************************************************************************
 * Helper functions for any_sir_event variant type                             *
 *******************************************************************************/


template<typename any_event, size_t event_index> struct event_size_by_event_index {
  constexpr static const size_t value = event_by_index<any_event, event_index>().affected_people.size();
};

struct any_sir_event_size {
  size_t operator()(const auto &x) const { return (x.affected_people.size()); }
};

struct any_sir_event_time {
  epidemic_time_t operator()(const auto &x) const { return (x.time); }
};

struct any_sir_event_preconditions {
  person_t &person;
  auto operator()(const auto &x) const { return (x.preconditions[person]); }
};

struct any_sir_event_affected_people {
  person_t &person;
  auto operator()(const auto &x) const { return (x.affected_people[person]); }
};

struct any_sir_event_postconditions {
  person_t &person;
  auto operator()(const auto &x) const { return (x.postconditions[person]); }
};

struct any_sir_event_set_time {
  epidemic_time_t value;
  void operator()(auto &x) const {
    x.time = value;
    return;
  }
};

struct any_sir_event_set_affected_people {
  person_t &position;
  person_t &value;
  void operator()(auto &x) const {
    x.affected_people[position] = value;
    return;
  }
};

struct any_sir_event_print {
  std::string &prefix;
  void operator()(const auto &x) const {
    std::cout << prefix;
    std::cout << "time " << x.time << " ";
    std::cout << "size " << x.affected_people.size() << " ";
    for (auto it : x.affected_people) { std::cout << it << ", "; }
    std::cout << std::endl;
  }
};

template<typename any_event, typename states_t>
struct any_state_check_preconditions {
  sir_state<states_t> &this_sir_state;
  template<size_t event_index> bool operator()(const event_by_index<any_event, event_index> x) const {
    auto rc = true;
    size_t event_size = x.affected_people.size();
    for (person_t i = 0; i < event_size; ++i) {
      auto tmp = false;
      // Make an iterator for states_t maybe
      for (size_t compartment = 0; compartment < std::size(states_t{}); ++compartment) {
        auto lhs = x.preconditions[i][compartment];
        auto rhs = this_sir_state.potential_states[x.affected_people[i]][compartment];
        tmp = tmp || (lhs && rhs);
      }
      rc = rc && tmp;
    }
    return (rc);
  }
};

template<typename states_t>
struct any_sir_event_apply_to_sir_state {
  sir_state<states_t> &this_sir_state;
  void operator()(const auto &x) const {
    this_sir_state.time = x.time;
    size_t event_size = x.affected_people.size();
    if (event_size == 0) { return; }

    for (auto person_index : std::ranges::views::iota(0UL) | std::ranges::views::take(event_size)) {
      auto to_state = x.postconditions[person_index];
      if (to_state) {
        auto affected_person = x.affected_people[person_index];
        for (auto previous_compartment :
	       std::ranges::views::iota(0UL) | std::ranges::views::take(std::size(states_t{}))) {
          this_sir_state.potential_states[affected_person][previous_compartment] = false;
        }
        this_sir_state.potential_states[affected_person][to_state.value()] = true;
      }
    }
  }
};

template<typename states_t>
struct any_event_apply_entered_states {
  sir_state<states_t> &this_sir_state;
  void operator()(const auto &x) const {
    this_sir_state.time = x.time;
    size_t event_size = x.affected_people.size();

    for (auto person_index : std::ranges::views::iota(0UL) | std::ranges::views::take(event_size)) {
      auto to_state = x.postconditions[person_index];
      if (to_state) {
        auto affected_person = x.affected_people[person_index];
        this_sir_state.potential_states[affected_person][to_state.value()] = true;
      }
    }
  }
};

template<typename states_t>
struct any_event_apply_left_states {
  sir_state<states_t> &this_sir_state;
  void operator()(const auto &x) const {
    this_sir_state.time = x.time;
    size_t event_size = x.affected_people.size();

    for (auto person_index : std::ranges::views::iota(0UL, event_size)) {
      auto to_state = x.postconditions[person_index];
      if (to_state) {
        for (auto state_index : std::ranges::views::iota(0UL, std::size(states_t{}))) {
          this_sir_state.potential_states[x.affected_people[person_index]][state_index] =
            this_sir_state.potential_states[x.affected_people[person_index]][state_index]
            && !x.preconditions[person_index][state_index];
        }
      }
    }
  }
};

/*******************************************************************************
 * Template helper functions for sir_events to use in generation               *
 *******************************************************************************/

/*
template<typename states_t, size_t event_index, size_t precondition_index>
bool check_event_precondition(std::array<bool, std::size(states_t{})>) {
  // bool rc = false;
  return (std::any_of(sir_event_true_preconditions<event_index, precondition_index>::value));
}
*/
/*
template<typename states_t, typename any_event, size_t event_index, size_t precondition_index>
bool check_event_precondition(std::array<bool, std::size(states_t{})> x) {
  bool rc = false;
  auto tmp = event_true_preconditions<any_event, event_index, precondition_index>::value;
  for (auto elem : tmp) { rc = rc || x[elem]; }
  return (rc);
}

template<typename states_t, typename any_event, size_t event_index, size_t precondition_index>
const auto check_event_precondition_enumerated = [](const auto &x) {
  return (check_event_precondition<states_t, any_event, event_index, precondition_index>(x.value));
};
*/

template<typename states_t, typename any_event, size_t event_index, size_t precondition_index>
const auto check_event_precondition_enumerated = [](const auto &x) {// x is a potential_states
  return ((x.value
           & std::variant_alternative_t<event_index, any_event>{}.preconditions[precondition_index])
            .any());
};

template<typename states_t, typename any_event, size_t event_index, size_t precondition_index>
const auto get_precondition_satisfying_indices(const sir_state<states_t> &current_state) {
  return (cor3ntin::rangesnext::to<std::vector>(
    cor3ntin::rangesnext::enumerate(current_state.potential_states)
    | std::ranges::views::filter(
				 check_event_precondition_enumerated<states_t, any_event, event_index, precondition_index>)
    | std::ranges::views::transform([](const auto &x) { return (x.index); })));
}

template<typename any_event, size_t event_index>
const auto transform_array_to_sir_event_l = [](const auto &x) {
  event_by_index<any_event, event_index> rc;
  std::apply([&rc](const auto... y) { rc.affected_people = { y... }; }, x);
  return (rc);
};

constexpr size_t int_pow(size_t base, size_t exponent) {
  if (exponent == 0) { return 1; }
  if (exponent == 1) { return base; }
  return (exponent % 2) == 0 ? int_pow(base, exponent / 2) * int_pow(base, exponent / 2)
                             : int_pow(base, exponent / 2) * int_pow(base, exponent / 2) * base;
}

template<typename states_t>
void print(const sir_state<states_t> &state, const std::string &prefix = "", bool aggregate = true) {
  std::cout << prefix << "Possible states at time ";
  std::cout << state.time << std::endl;
  // return;
  if (!aggregate) {
    int person_counter = 0;
    for (auto possible_states : state.potential_states) {
      std::cout << prefix << person_counter << "(";
      std::cout << possible_states << "\n";
      std::cout << " )" << std::endl;
      ++person_counter;
    }
    return;
  }

  std::array<size_t, int_pow(2, std::size(states_t{})) + 1> aggregates;
  for (size_t subset = 0; subset <= int_pow(2, std::size(states_t{})); ++subset) { aggregates[subset] = 0; }
  for (auto possible_states : state.potential_states) {
    size_t index = 0;
    for (size_t state_index = 0; state_index < std::size(states_t{}); ++state_index) {
      if (possible_states[state_index]) { index += int_pow(2, state_index); }
    }
    aggregates[index]++;
  }
  for (size_t subset = 0; subset <= int_pow(2, std::size(states_t{})); ++subset) {
    if (aggregates[subset] > 0) {
      std::cout << prefix;
      for (size_t compartment = 0; compartment < std::size(states_t{}); ++compartment) {
        if ((subset % int_pow(2, compartment + 1) / int_pow(2, compartment)) == 1) {
          std::cout << compartment << " ";
        }
      }
      std::cout << " : " << aggregates[subset] << std::endl;
    }
  }
}

const auto apply_lambda = [](const auto &...x) { return cor3ntin::rangesnext::product(x...); };

template<
  typename states_t,
  typename any_event,
  size_t event_index,
  typename = std::make_index_sequence<event_size_by_event_index<any_event, event_index>::value>
  >
struct single_type_event_generator;

template<
  typename states_t,
  typename any_event,
  size_t event_index,
  size_t... precondition_index
  >
struct single_type_event_generator<states_t, any_event, event_index, std::index_sequence<precondition_index...>> {
  std::tuple<repeat<std::vector<size_t>, event_size_by_event_index<any_event, precondition_index>::value>...>
    vectors;
  auto cartesian_range() {
    // return(std::apply(apply_lambda, vectors));
    return (std::apply(cor3ntin::rangesnext::product, vectors));
  }
  auto event_range() {
    return (
	    std::ranges::transform_view(cartesian_range(), transform_array_to_sir_event_l<any_event, event_index>));
  }
  explicit single_type_event_generator(const sir_state<states_t> &current_state)
    : vectors{ std::make_tuple(
			       get_precondition_satisfying_indices<states_t, any_event, event_index, precondition_index>(current_state)...) } {}
};

/*
template<typename states_t, typename T, size_t N, typename = std::make_index_sequence<N>>
struct any_event_type_range;

template<typename states_t, typename T, size_t N, size_t... event_index>
struct any_event_type_range<states_t, T, N, std::index_sequence<event_index...>>
  : std::variant<decltype(single_type_event_generator<states_t, event_index>().event_range())...> {};

template <size_t N = number_of_event_types,
          typename = std::make_index_sequence<N>>
struct single_time_event_generator;

template <size_t N, size_t... event_index>
struct single_time_event_generator<N, std::index_sequence<event_index...>> {

  std::tuple<cartesian_multiproduct<event_index>...> product_thing;

  // constexpr const static auto concat_lambda = [](auto &...x) {
  //  return (ranges::views::concat(x.event_range()...));
  //};

  std::variant<decltype(std::get<event_index>(product_thing).event_range())...>
      range_variant;

    cppcoro::generator<any_sir_event> test(){
    for(any_sir_event elem : std::get<0>(product_thing).event_range()){
    co_yield elem;
    }
    }

  auto event_range() { return (std::apply(concat_lambda, product_thing)); }

  single_time_event_generator() = default;
  single_time_event_generator(sir_state &initial_conditions)
      : product_thing(
            cartesian_multiproduct<event_index>(initial_conditions)...){}
};
*/

#endif

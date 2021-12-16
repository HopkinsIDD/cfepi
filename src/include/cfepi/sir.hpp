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


//! \defgroup General_Purpose_Code General Purpose Code
namespace cfepi {

namespace detail {
  template<typename T, size_t i> using repeat = T;
  constexpr size_t int_pow(size_t base, size_t exponent) {
    if (exponent == 0) { return 1; }
    if (exponent == 1) { return base; }
    return (exponent % 2) == 0
             ? int_pow(base, exponent / 2) * detail::int_pow(base, exponent / 2)
             : int_pow(base, exponent / 2) * detail::int_pow(base, exponent / 2) * base;
  }
}// namespace detail

  //! @defgroup generalconcepts General Concepts
}// namespace cfepi

namespace cfepi {
//! \defgroup compartment_model Compartmental Models
//!@{

// REMOVE ME
typedef long long epidemic_time_t;
// REMOVE ME
typedef size_t person_t;


/*******************************************************************************
 * SIR Helper Type Definitions                                                 *
 *******************************************************************************/

//! @defgroup is_sized_enum Sized Enum
//! @ingroup generalconcepts
//! @{
template<typename enum_type>
concept is_sized_enum = requires(enum_type e) {
  // { e.state };
  { std::size(e) } -> std::unsigned_integral;
}
&&std::default_initializable<enum_type> &&std::is_enum<typename enum_type::state>::value;
//! @}

/*******************************************************************************
 * SIR State Definition                                                        *
 *******************************************************************************/

/*
 * @name State
 * @description The state of the model.
 */

//! \brief Class for keeping track of the current state of a compartmental model (with potential states).
//!
//! Has a few methods and operators, but is essentially just a std::vector<std::bitset<std::size(states_t{})>> containing potential states

template<typename states_t>
requires is_sized_enum<states_t>
struct sir_state {
  //! \brief Main part of the class. A state representation for each person, representing whether they could be part of each state in states_t
  std::vector<std::bitset<std::size(states_t{})>> potential_states;
  //! \brief Time that this state represents
  epidemic_time_t time = -1;
  //! \brief Default constructor with size 0.
  sir_state() noexcept = default;
  //! \brief Default constructor by size.
  sir_state(person_t _population_size) noexcept { potential_states.resize(_population_size); }
  //! \brief The or operator applies to potential states, so elementwise or on potential_states
  sir_state operator||(const sir_state &other) const {
    sir_state rc{ *this };
    if (rc.potential_states.size() != other.potential_states.size()) {
      throw "Cannot compare sir_states with different sizes";
    }

    std::ranges::transform(this->potential_states,
      other.potential_states,
      std::begin(rc.potential_states),
      std::bit_or<>(),
      {},
      {});
    return (rc);
  }

  //! \brief Set all potential states to false
  void reset() {
    for (auto &i : potential_states) {
      i.reset();
    }
  }

};


//! \brief Class for keeping track of the current state of a compartmental model (without potential states).
//!
//! Has an == operator for testing, but otherwise just a wrapper for potential_state_counts.
//! There is also an external print method
template<typename states_t> struct aggregated_sir_state {
  //! \brief The main part of the class. For each element of the powerset of states_t, stores counts of people in that potential states
  std::array<size_t, detail::int_pow(2, std::size(states_t{})) + 1> potential_state_counts;
  //! \brief Time that this state represents
  epidemic_time_t time = -1;
  //! \brief Default constructor with size 0
  aggregated_sir_state() = default;
  //! \brief Default constructor by component
  aggregated_sir_state(const std::array<size_t, detail::int_pow(2, std::size(states_t{})) + 1>
                         &_potential_state_counts,
    epidemic_time_t _time)
    : potential_state_counts(_potential_state_counts), time(_time){};
  //! \brief Conversion constructor from states with potential states
  // Wrapper for aggregate_state_to_array
  aggregated_sir_state(const sir_state<states_t> &sir_state)
    : potential_state_counts(aggregate_state_to_array(sir_state)), time(sir_state.time){};
  //! \brief basically only for testing
  bool operator==(const aggregated_sir_state<states_t>& other) const {
    bool rc = true;
    for(auto i : std::views::iota(0UL, detail::int_pow(2, std::size(states_t{})) + 1)) {
      rc = rc && (other.potential_state_counts[i] == potential_state_counts[i]);
      if (!rc) {
	print(other);
	print(*this);
	return(false);
      }
    };
    return rc;
  };
};

/*!
 * \fn is_simple
 * \brief Determine if an sir_state represents a single world.
 *
 * \param state The state to check
 * \return Returns true if every person in the state has exactly one potential state, otherwise returns false
 */
template<typename states_t> bool is_simple(const aggregated_sir_state<states_t> &state) {
  size_t pow_idx = 1;
  bool rc = true;
  for (auto idx : std::views::iota(1UL, detail::int_pow(2, std::size(states_t{})) + 1)) {
    if (idx != pow_idx) {
      rc = rc && state.potential_state_counts[idx] == 0;
    } else {
      pow_idx *= 2;
    }
  }
  return (rc);
}


//! \brief
template<typename states_t>
sir_state<states_t> default_state(const typename states_t::state base_state,
  const typename states_t::state infected_state,
  const person_t population_size = 10,
  const person_t initial_infected = 1) {
  sir_state<states_t> rc{ population_size };
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

template<typename states_t> struct interaction_event : public sir_event<states_t, 2> {
  using sir_event<states_t, 2>::time;
  using sir_event<states_t, 2>::affected_people;
  using sir_event<states_t, 2>::preconditions;
  using sir_event<states_t, 2>::postconditions;
  interaction_event(const interaction_event &) = default;
  constexpr interaction_event(person_t p1,
    person_t p2,
    epidemic_time_t _time,
    const std::bitset<std::size(states_t{})> preconditions_for_first_person,
    const std::bitset<std::size(states_t{})> preconditions_for_second_person,
    const typename states_t::state result_state) {
    time = _time;
    affected_people = { p1, p2 };
    preconditions = { preconditions_for_first_person, preconditions_for_second_person };
    postconditions[0] = result_state;
  }
  constexpr interaction_event(
    const std::array<bool, std::size(states_t{})> preconditions_for_first_person,
    const std::array<bool, std::size(states_t{})> preconditions_for_second_person,
    const typename states_t::state result_state) noexcept
    : interaction_event(0UL,
      0UL,
      -1,
      preconditions_for_first_person,
      preconditions_for_second_person,
      result_state){};
};

template<typename states_t> struct transition_event : public sir_event<states_t, 1> {
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
  constexpr transition_event(const std::array<bool, std::size(states_t{})> _preconditions,
    const typename states_t::state result_state) noexcept
    : transition_event(0, -1, _preconditions, result_state) {}
};

template<typename states_t, size_t N, typename = std::make_index_sequence<N>>
struct sir_event_constructor;

/*******************************************************************************
 * Helper functions for any_sir_event variant type                             *
 *******************************************************************************/


template<typename any_event, size_t event_index> struct event_size_by_event_index {
  constexpr static const size_t value =
    std::variant_alternative_t<event_index, any_event>{}.affected_people.size();
};

struct any_sir_event_print {
  std::string prefix;
  void operator()(const auto &x) const {
    std::cout << prefix;
    std::cout << "time " << x.time << " ";
    std::cout << "size " << x.affected_people.size() << " ";
    for (auto it : x.affected_people) { std::cout << it << ", "; }
    std::cout << std::endl;
  }
};

template<typename any_event, typename states_t> struct any_state_check_preconditions {
  sir_state<states_t> &this_sir_state;
  bool operator()(const auto x) const {
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

template<typename states_t> struct any_event_apply_entered_states {
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

template<typename states_t> struct any_event_apply_left_states {
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
  std::variant_alternative_t<event_index, any_event> rc;
  std::apply([&rc](const auto... y) { rc.affected_people = { y... }; }, x);
  return (rc);
};


template<typename states_t>
std::array<size_t, detail::int_pow(2, std::size(states_t{})) + 1> aggregate_state_to_array(
  const sir_state<states_t> &state) {
  std::array<size_t, detail::int_pow(2, std::size(states_t{})) + 1> rc;
  for (auto &elem : rc) { elem = 0; }
  for (auto possible_states : state.potential_states) { rc[possible_states.to_ulong()] += 1; }
  return (rc);
}

template<typename states_t>
aggregated_sir_state<states_t> aggregate_state(const sir_state<states_t> &state) {
  return (aggregated_sir_state{ state });
}

template<typename states_t>
void print(const sir_state<states_t> &state,
  const std::string &prefix = "",
  bool aggregate = true) {
  if (aggregate) {
    print(aggregate_state(state), prefix);
    return;
  }
  std::cout << prefix << "Possible states at time ";
  std::cout << state.time << std::endl;
  int person_counter{ 0 };
  for (auto possible_states : state.potential_states) {
    std::cout << prefix << person_counter << "(";
    std::cout << possible_states << "\n";
    std::cout << " )" << std::endl;
    ++person_counter;
  }
}

template<typename states_t>
void print(const aggregated_sir_state<states_t> &aggregates, const std::string &prefix = "") {
  std::cout << prefix << "Possible states at time ";
  std::cout << aggregates.time << "\n";
  for (size_t subset = 0; subset <= detail::int_pow(2, std::size(states_t{})); ++subset) {
    if (aggregates.potential_state_counts[subset] > 0) {
      std::cout << prefix;
      for (size_t compartment = 0; compartment < std::size(states_t{}); ++compartment) {
        if ((subset % detail::int_pow(2, compartment + 1) / detail::int_pow(2, compartment)) == 1) {
          std::cout << compartment << " ";
        }
      }
      std::cout << " : " << aggregates.potential_state_counts[subset] << std::endl;
    }
  }
}

const auto apply_lambda = [](const auto &...x) { return cor3ntin::rangesnext::product(x...); };

template<typename states_t,
  typename any_event,
  size_t event_index,
  typename = std::make_index_sequence<event_size_by_event_index<any_event, event_index>::value>>
struct single_type_event_generator;

template<typename states_t, typename any_event, size_t event_index, size_t... precondition_index>
struct single_type_event_generator<states_t,
  any_event,
  event_index,
  std::index_sequence<precondition_index...>> {
  std::tuple<detail::repeat<std::vector<size_t>,
    event_size_by_event_index<any_event, precondition_index>::value>...>
    vectors;
  auto cartesian_range() {
    // return(std::apply(apply_lambda, vectors));
    return (std::apply(cor3ntin::rangesnext::product, vectors));
  }
  auto event_range() {
    return (std::ranges::transform_view(
      cartesian_range(), transform_array_to_sir_event_l<any_event, event_index>));
  }
  explicit single_type_event_generator(const sir_state<states_t> &current_state)
    : vectors{ std::make_tuple(
      get_precondition_satisfying_indices<states_t, any_event, event_index, precondition_index>(
        current_state)...) } {}
};

/*
template<typename states_t, typename T, size_t N, typename = std::make_index_sequence<N>>
struct any_event_type_range;

template<typename states_t, typename T, size_t N, size_t... event_index>
struct any_event_type_range<states_t, T, N, std::index_sequence<event_index...>>
  : std::variant<decltype(single_type_event_generator<states_t, event_index>().event_range())...>
{};

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

//!@}
}// namespace cfepi

#endif

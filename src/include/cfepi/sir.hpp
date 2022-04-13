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
#include <numeric>
#include <concepts>

// #include <range/v3/all.hpp>
// #include <stxxl/vector>

#include <cor3ntin/rangesnext/to.hpp>
#include <cor3ntin/rangesnext/product.hpp>
#include <cor3ntin/rangesnext/enumerate.hpp>

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
  // { state };
  { std::size(e) } -> std::unsigned_integral;
}
&&std::default_initializable<enum_type>;//&&std::is_enum<typename enum_type::state>::value;
//! @}

/*******************************************************************************
 * SIR State Definition                                                        *
 *******************************************************************************/

/*
 * @name State
 * @description The state of the model.
 */

//! \brief Class for keeping track of the current state of a compartmental model (with potential
//! states).
//!
//! Has a few methods and operators, but is essentially just a
//! std::vector<std::bitset<std::size(states_t{})>> containing potential states

template<typename states_t>
requires is_sized_enum<states_t>
struct sir_state {
  //! \brief Main part of the class. A state representation for each person, representing whether
  //! they could be part of each state in states_t
  std::vector<std::bitset<std::size(states_t{})>> potential_states;
  //! \brief Time that this state represents
  epidemic_time_t time;
  //! \brief Default constructor with size 0.
  sir_state() noexcept = default;
  //! \brief Default constructor by size.
  explicit sir_state(person_t _population_size) noexcept
    : potential_states(_population_size), time(-1) {}
  sir_state(const sir_state &other) = default;
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

  size_t size() const {
    return(std::size(potential_states));
  }

  //! \brief Set all potential states to false
  void reset() {
    for (auto &i : potential_states) { i.reset(); }
  }
};


//! \brief Class for keeping track of the current state of a compartmental model (without potential
//! states).
//!
//! Has an == operator for testing, but otherwise just a wrapper for potential_state_counts.
//! There is also an external print method
template<typename states_t> struct aggregated_sir_state {
  //! \brief The main part of the class. For each element of the powerset of states_t, stores counts
  //! of people in that potential states
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
  explicit aggregated_sir_state(const sir_state<states_t> &sir_state)
    : potential_state_counts(aggregate_state_to_array(sir_state)), time(sir_state.time){};
  //! \brief basically only for testing
  bool operator==(const aggregated_sir_state<states_t> &other) const {
    return (std::transform_reduce(
      std::begin(potential_state_counts),
      std::end(potential_state_counts),
      std::begin(other.potential_state_counts),
      true,
      [](const auto &x, const auto &y) { return (x || y); },
      [](const auto &x, const auto &y) { return (x == y); }));
  };
};

/*!
 * \fn is_simple
 * \brief Determine if an sir_state represents a single world.
 *
 * \param state The state to check
 * \return Returns true if every person in the state has exactly one potential state, otherwise
 * returns false
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

template<typename states_t, size_t event_size> struct sir_event_type {
  std::array<std::bitset<std::size(states_t{})>, event_size> preconditions = {};
  std::array<std::optional<typename states_t::state>, event_size> postconditions;
  constexpr auto static inline size() { return (event_size); };
  typedef states_t state_type;
};

template<typename T>
concept is_event_type_like =
  std::same_as<typename T::state_type, typename T::state_type> && requires(T e, size_t i) {
  { T::size() } -> std::unsigned_integral;
  { e.preconditions[i][i] } -> std::convertible_to<bool>;
  // { e.postconditions[i] } -> std::convertible_to<typename T::state_type>;
  { std::size(e) } -> std::unsigned_integral;
};

template<typename event_type>
requires is_event_type_like<event_type>
struct sir_event {
  epidemic_time_t time = -1;
  std::array<person_t, event_type::size()> affected_people = {};
  event_type type;
  sir_event() noexcept = default;
  sir_event(const sir_event &) = default;
  explicit sir_event(const sir_event_type<typename event_type::state_type, event_type::size()> &other)
    : time(-1), affected_people({}),
      type(sir_event_type<typename event_type::state_type, event_type::size()>(other)){};
};

template<typename states_t> struct interaction_event_type : public sir_event_type<states_t, 2> {
  using sir_event_type<states_t, 2>::preconditions;
  using sir_event_type<states_t, 2>::postconditions;
  interaction_event_type(const interaction_event_type &) = default;
  constexpr interaction_event_type(
    const std::bitset<std::size(states_t{})> &preconditions_for_first_person,
    const std::bitset<std::size(states_t{})> &preconditions_for_second_person,
    const typename states_t::state result_state) {
    preconditions = { preconditions_for_first_person, preconditions_for_second_person };
    postconditions[0] = result_state;
  }
};

template<typename states_t> struct transition_event_type : public sir_event_type<states_t, 1> {
  using sir_event_type<states_t, 1>::preconditions;
  using sir_event_type<states_t, 1>::postconditions;
  transition_event_type(const transition_event_type &) = default;
  constexpr transition_event_type(const std::bitset<std::size(states_t{})> &_preconditions,
    const states_t::state result_state) {
    preconditions = { _preconditions };
    postconditions[0] = result_state;
  }
};

template<typename states_t, size_t N, typename = std::make_index_sequence<N>>
struct sir_event_constructor;

/*******************************************************************************
 * Helper functions for any_sir_event variant type                             *
 *******************************************************************************/


template<typename any_event_type, size_t event_index> struct event_size_by_event_index {
  constexpr static const size_t value =
    std::size(std::variant_alternative_t<event_index, any_event_type>{});
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

template<typename states_t> struct any_sir_event_get_affected_person {
  size_t index;
  template<typename event_type> person_t operator()(const sir_event<event_type> &x) const {
    return x.affected_people[index];
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
        auto lhs = x.type.preconditions[i][compartment];
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
      auto to_state = x.type.postconditions[person_index];
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
      auto to_state = x.type.postconditions[person_index];
      if (to_state) {
        for (auto state_index : std::ranges::views::iota(0UL, std::size(states_t{}))) {
          this_sir_state.potential_states[x.affected_people[person_index]][state_index] =
            this_sir_state.potential_states[x.affected_people[person_index]][state_index]
            && !x.type.preconditions[person_index][state_index];
        }
      }
    }
  }
};

/*******************************************************************************
 * Template helper functions for sir_events to use in generation               *
 *******************************************************************************/

template<typename states_t, typename event_type, size_t precondition_index>
struct check_event_precondition_enumerated {
  const event_type &event;
  auto operator()(const auto &x) {// x is a potential_states
    auto rc = (x.value & event.preconditions[precondition_index]).any();
    return (rc);
  }
};

template<typename states_t, size_t precondition_index>
const auto get_precondition_satisfying_indices(const auto &event,
  const sir_state<states_t> &current_state) {
  return (cor3ntin::rangesnext::to<std::vector>(
    cor3ntin::rangesnext::enumerate(current_state.potential_states)
    | std::ranges::views::filter(
      check_event_precondition_enumerated<states_t, decltype(event), precondition_index>{ event })
    | std::ranges::views::transform([](const auto &x) { return (x.index); })));
}

template<typename event_type_t>
const auto transform_array_to_sir_event_l = [](const event_type_t &event_type, const auto &x) {
  sir_event<event_type_t> rc;
  rc.type = event_type;
  std::apply([&rc](const auto... y) { rc.affected_people = { y... }; }, x);
  return (rc);
};

/*
template <typename event_type, size_t event_index>
struct transform_array_to_sir_event_l {
const event_type& type;
const auto
operator()(const auto &x) {
  // std::variant_alternative_t<event_index, any_event_type> rc;
  // sir_event rc<std::variant_alternative_t<event_index, any_event_type>>
  // rc{};
  sir_event<event_type> rc{type};
  std::apply([&rc](const auto... y) { rc.affected_people = {y...}; }, x);
  return (rc);
}
};
*/

template<typename states_t>
std::array<size_t, detail::int_pow(2, std::size(states_t{})) + 1> aggregate_state_to_array(
  const sir_state<states_t> &state) {
  std::array<size_t, detail::int_pow(2, std::size(states_t{})) + 1> rc;
  std::fill(std::begin(rc), std::end(rc), 0);
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

const auto apply_lambda = [](const auto &...x) {
  static_assert(
    (std::same_as<decltype(x), const std::vector<size_t> &> && ...), "This should not happen\n");
  return cor3ntin::rangesnext::product(x...);
};

template<typename any_event_type,
  typename = std::make_index_sequence<std::variant_size_v<any_event_type>>>
struct any_event;

template<typename any_event_type, size_t... event_index>
struct any_event<any_event_type, std::index_sequence<event_index...>> {
  typedef std::variant<sir_event<std::variant_alternative_t<event_index, any_event_type>>...> type;
};

template<typename any_event_type,
  typename = std::make_index_sequence<std::variant_size_v<any_event_type>>>
struct all_event_types;

template<typename any_event_type, size_t... event_index>
struct all_event_types<any_event_type, std::index_sequence<event_index...>>
  : std::tuple<std::variant_alternative_t<event_index, any_event_type>...> {};

template<typename event_type_t, typename = std::make_index_sequence<event_type_t::size()>>
struct single_type_event_generator;

template<typename event_type_t, size_t... precondition_index>
struct single_type_event_generator<event_type_t, std::index_sequence<precondition_index...>> {
  event_type_t event_type;
  std::tuple<detail::repeat<std::vector<size_t>, precondition_index>...> vectors;

  // Making this const causes everything to fail
  auto cartesian_range() {
    return (std::apply(cor3ntin::rangesnext::product, vectors));
    // return (std::apply(apply_lambda, vectors));
  }

  auto event_range() {
    const auto local_event_type = event_type;
    const auto the_lambda = [local_event_type](const auto &x) {
      return (transform_array_to_sir_event_l<event_type_t>(local_event_type, x));
      // return(sir_event<event_type_t>{});
    };
    return (std::ranges::transform_view(cartesian_range(), the_lambda));
  }

  explicit single_type_event_generator(event_type_t event_type_,
    const sir_state<typename event_type_t::state_type> &current_state)
    : event_type(event_type_),
      vectors(std::make_tuple(
        get_precondition_satisfying_indices<typename event_type_t::state_type, precondition_index>(
          event_type,
          current_state)...)) {
    // This size check is conditioned on the cartesian product of a non-empty range and an empty range segfaulting
    // There is a commented out test called "Product view works for a non-empty and empty vector", which verifies this is still happening
    std::vector<size_t> all_vector_sizes =
      std::apply([](const auto&... x){ return (std::vector<size_t>{ std::size(x)... }); }, vectors);
    for (auto this_size : all_vector_sizes) {
      if (this_size == 0) {
	std::apply([](auto&... x) { (..., x.clear());}, vectors);
      }
    }
  }
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

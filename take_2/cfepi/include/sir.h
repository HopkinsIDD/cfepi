#include <chrono>   // std::chrono::seconds
#include <iostream> // std::cout, std::endl
#include <thread>   // std::this_thread::sleep_for
// this_thread::yield example
#include <atomic> // std::atomic
#include <functional>
#include <mutex>
#include <queue>
#include <typeinfo>
#include <variant>
#include <vector>

#include <range/v3/all.hpp>
// #include <stxxl/vector>

#ifndef __SIR_H_
#define __SIR_H_

/*******************************************************************************
 * Generic functions                                                           *
 *******************************************************************************/

template<typename T,size_t i>
using repeat = T;


/*******************************************************************************
 * SIR Helper Type Definitions                                                 *
 *******************************************************************************/

enum epidemic_state { S, I, R, n_SIR_compartments };
constexpr size_t ncompartments = n_SIR_compartments;
typedef float epidemic_time_t;
typedef size_t person_t;
size_t global_event_counter = 0;

/*******************************************************************************
 * SIR State Definition                                                        *
 *******************************************************************************/

/*
 * @name State
 * @description The state of the model.
 */

struct sir_state {
  person_t population_size = 0;
  std::vector<std::array<bool, ncompartments>> potential_states;
  std::vector<bool> states_modified;
  epidemic_time_t time = -1;
  float beta = .2;
  float gamma = .1;
  std::string prefix;
  sir_state() noexcept = default;
  sir_state(person_t population_size = 10) noexcept
      : population_size(population_size) {
    potential_states.resize(population_size);
    states_modified.resize(population_size);
  }
};

sir_state default_state(person_t population_size = 10,
                        person_t initial_infected = 1) {
  sir_state rc(population_size);
  auto base_range = ranges::views::iota(population_size * 0, population_size);
  for (auto elem : base_range | ranges::views::take(initial_infected)) {
    rc.potential_states[elem][I] = true;
  }
  rc.potential_states[0][I] = true;
  for (auto elem : base_range | ranges::views::drop(initial_infected)) {
    rc.potential_states[elem][S] = true;
  }
  return (rc);
}

auto sample_sir_state = default_state(1, 0);

/*******************************************************************************
 * SIR Event Definition                                                        *
 *******************************************************************************/

/*
 * @name SIR Event
 * @description An event that transitions the model between states
 */

template <size_t size> struct sir_event {
  epidemic_time_t time = -1;
  std::array<person_t, size> affected_people = {};
  std::array<std::array<bool, ncompartments>, size> preconditions = {};
  std::array<std::optional<epidemic_state>, size> postconditions;
  sir_event() noexcept = default;
  sir_event(const sir_event &) = default;
};

struct infection_event : public sir_event<2> {
  using sir_event<2>::time;
  using sir_event<2>::affected_people;
  using sir_event<2>::preconditions;
  using sir_event<2>::postconditions;
  constexpr infection_event() noexcept {
    preconditions = {std::array<bool, 3>({true, false, false}),
                     std::array<bool, 3>({false, true, false})};
    postconditions[0] = I;
  }
  infection_event(const infection_event &) = default;
  infection_event(person_t p1, person_t p2, epidemic_time_t _time) {
    time = _time;
    affected_people = {p1, p2};
    preconditions = {std::array<bool, 3>({true, false, false}),
                     std::array<bool, 3>({false, true, false})};
    postconditions[0] = I;
  }
};

struct recovery_event : public sir_event<1> {
  using sir_event<1>::time;
  using sir_event<1>::affected_people;
  using sir_event<1>::preconditions;
  using sir_event<1>::postconditions;
  constexpr recovery_event() noexcept {
    preconditions = {std::array<bool, 3>({false, true, false})};
    postconditions[0] = R;
  }
  recovery_event(const recovery_event &) = default;
  recovery_event(person_t p1, epidemic_time_t _time) {
    time = _time;
    affected_people[0] = p1;
    preconditions = {std::array<bool, 3>({false, true, false})};
    postconditions[0] = R;
  }
};

struct null_event : public sir_event<0> {
  using sir_event<0>::time;
  using sir_event<0>::affected_people;
  using sir_event<0>::preconditions;
  using sir_event<0>::postconditions;
  null_event() noexcept = default;
  null_event(const null_event &) = default;
  null_event(epidemic_time_t _time) { time = _time; }
};

typedef std::variant<recovery_event, infection_event, null_event> any_sir_event;
constexpr size_t number_of_event_types = 3;

template <size_t index, size_t permutation_index>
struct sir_event_true_preconditions;

template <> struct sir_event_true_preconditions<1, 0> {
  constexpr static auto value = {I};
};

template <> struct sir_event_true_preconditions<2, 0> {
  constexpr static auto value = {S};
};

template <> struct sir_event_true_preconditions<2, 1> {
  constexpr static auto value = {I};
};

template <size_t event_index> struct sir_event_by_index;

template <> struct sir_event_by_index<0> : null_event {};

template <> struct sir_event_by_index<1> : recovery_event {};

template <> struct sir_event_by_index<2> : infection_event {};

template <size_t N = number_of_event_types,
          typename = std::make_index_sequence<N>>
struct sir_event_constructor;

template <size_t N, size_t... indices>
struct sir_event_constructor<N, std::index_sequence<indices...>> {
  any_sir_event construct_sir_event(size_t event_index) {
    any_sir_event rc;
    bool any_possible = (false | ... | (event_index == indices));
    if (!any_possible) {
      std::cerr << "Printing index was out of bounds" << std::endl;
      return rc;
    }
    ((rc = (event_index == indices) ? sir_event_by_index<indices>() : rc), ...);
    return rc;
  }
};


/*******************************************************************************
 * Helper functions for any_sir_event variant type                             *
 *******************************************************************************/

template <size_t event_index> struct event_size_by_event_index {
  const static size_t value =
      sir_event_by_index<event_index>().affected_people.size();
};

struct any_sir_event_size {
  size_t operator()(const auto &x) { return (x.affected_people.size()); }
};

struct any_sir_event_time {
  epidemic_time_t operator()(const auto &x) { return (x.time); }
};

struct any_sir_event_preconditions {
  person_t &person;
  auto operator()(const auto &x) { return (x.preconditions[person]); }
};

struct any_sir_event_affected_people {
  person_t &person;
  auto operator()(const auto &x) { return (x.affected_people[person]); }
};

struct any_sir_event_postconditions {
  person_t &person;
  auto operator()(const auto &x) { return (x.postconditions[person]); }
};

struct any_sir_event_set_time {
  epidemic_time_t value;
  void operator()(auto &x) {
    x.time = value;
    return;
  }
};

struct any_sir_event_set_affected_people {
  person_t &position;
  person_t &value;
  void operator()(auto &x) {
    x.affected_people[position] = value;
    return;
  }
};

struct any_sir_event_print {
  std::string &prefix;
  void operator()(const auto &x) {
    std::cout << prefix;
    std::cout << "time " << x.time << " ";
    std::cout << "size " << x.affected_people.size() << " ";
    for (auto it : x.affected_people) {
      std::cout << it << ", ";
    }
    std::cout << std::endl;
  }
};

struct any_sir_state_check_preconditions {
  sir_state& sir_state;
  bool operator()(const auto &x) {
    auto rc = true;
    auto tmp = false;
    size_t event_size = x.affected_people.size();
    for(person_t i = 0; i < event_size; ++i){
      tmp = false;
      for(size_t compartment = 0; compartment < ncompartments; ++compartment){
	auto lhs = x.preconditions[compartment];
	auto rhs = sir_state.potential_states[x.affected_people[i] ][compartment];
	tmp = tmp || (lhs && rhs);
      }
      rc = rc && tmp;
    }
    return(rc);
  }
};

struct any_sir_event_apply_to_sir_state {
  sir_state& sir_state;
  void operator()(const auto &x) {
    sir_state.time = x.time;
    size_t event_size = x.affected_people.size();
    if(event_size == 0){
      return;
    }

    for(auto person_index : ranges::views::iota( 0UL ) | ranges::views::take(event_size)){
      auto to_state = x.postconditions[person_index];
      if(to_state){
	auto affected_person = x.affected_people[person_index];
	for(auto previous_compartment : ranges::views::iota( 0UL) | ranges::views::take(ncompartments)){
	  sir_state.potential_states[affected_person][previous_compartment] = false;
	}
	sir_state.potential_states[affected_person][to_state.value()] = true;
      }
    }
  }
};

/*******************************************************************************
 * Template helper functions for sir_events to use in generation               *
 *******************************************************************************/

template<size_t event_index, size_t precondition_index>
bool check_event_precondition(std::array<bool, number_of_event_types> x){
  bool rc = false;
  auto tmp = sir_event_true_preconditions<event_index, precondition_index>::value;
  for(auto elem : tmp){
    rc = rc || x[elem];
  }
  return(rc);
}

template<
  size_t event_index,
  size_t precondition_index
  >
struct filter_population_by_event_preconditions{
  static auto get_filtered_reference_vectors(const auto & single_gen){
    auto vector_start = ranges::begin(single_gen);

    return(ranges::to<std::vector<size_t> >(
	ranges::transform_view(
	  ranges::filter_view(single_gen, &check_event_precondition<event_index,precondition_index>),
	  [&vector_start](auto & x){ return(&x - vector_start); }
	)
      ));
  }
};

template<size_t event_index>
const auto transform_array_to_sir_event_l = [](auto&& x){
  sir_event_by_index<event_index> rc;
  std::apply([&rc](auto... y){rc.affected_people = {y...};}, x);
  return((any_sir_event) rc);
 };

/*
 * Printing helpers
 */

void print(const any_sir_event &event, std::string prefix = "") {
  std::visit(any_sir_event_print{prefix}, event);
}

constexpr size_t int_pow(size_t base, size_t exponent) {
  if (exponent == 0) {
    return 1;
  }
  if (exponent == 1) {
    return base;
  }
  size_t sqrtfloor = int_pow(base, exponent / 2);
  if ((exponent % 2) == 0) {
    return (sqrtfloor * sqrtfloor);
  }
  return (sqrtfloor * sqrtfloor * base);
}

void print(const sir_state &state, std::string prefix = "",
           bool aggregate = true) {
  std::cout << prefix << "Possible states at time ";
  std::cout << state.time << std::endl;
  int person_counter = 0;
  int state_count = ncompartments;
  // return;
  if (!aggregate) {
    for (auto possible_states : state.potential_states) {
      std::cout << prefix << person_counter << "(";
      state_count = 0;
      for (auto this_state : possible_states) {
        if (this_state) {
          std::cout << " " << state_count;
        }
        ++state_count;
      }
      std::cout << " )" << std::endl;
      ++person_counter;
    }
    return;
  }

  std::array<size_t, int_pow(2, ncompartments)+1> aggregates;
  for (size_t subset = 0; subset <= int_pow(2, ncompartments); ++subset) {
    aggregates[subset] = 0;
  }
  for (auto possible_states : state.potential_states) {
    size_t index = 0;
    for (size_t state_index = 0; state_index < ncompartments; ++state_index) {
      if (possible_states[state_index]) {
        index += int_pow(2, state_index);
      }
    }
    aggregates[index]++;
  }
  for (size_t subset = 0; subset <= int_pow(2, ncompartments); ++subset) {
    if (aggregates[subset] > 0) {
      std::cout << prefix;
      for (size_t compartment = 0; compartment < ncompartments; ++compartment) {
        if ((subset % int_pow(2, compartment + 1) / int_pow(2, compartment)) ==
            1) {
          std::cout << compartment << " ";
        }
      }
      std::cout << " : " << aggregates[subset] << std::endl;
    }
  }
}

template<size_t event_index, typename = std::make_index_sequence<event_size_by_event_index<event_index>::value > >
struct cartesian_multiproduct;

template<size_t event_index, size_t... precondition_index>
struct cartesian_multiproduct<event_index,std::index_sequence<precondition_index...> >{
  std::tuple<repeat<std::vector<size_t>, event_size_by_event_index<precondition_index>::value>... > vectors;
  constexpr const static auto apply_lambda = [](auto&... x){return(ranges::views::cartesian_product(x...));};
  auto cartesian_range(){
    return(std::apply(apply_lambda, vectors));
  }
  auto event_range(){
    return(ranges::transform_view(cartesian_range(), transform_array_to_sir_event_l<event_index>));
  }
  // cartesian_multiproduct(std::tuple<repeat<std::vector<size_t>, event_size_by_event_index<precondition_index>::value>... > _vectors) : vectors(std::move(_vectors)) {};
  // cartesian_multiproduct(const cartesian_multiproduct& _from) : vectors(std::move(_from.vectors)) {};
  cartesian_multiproduct() = default;
  cartesian_multiproduct(const sir_state& current_state) {
    auto count_gen = ranges::span(current_state.potential_states);
    vectors = std::make_tuple(filter_population_by_event_preconditions<event_index,precondition_index>::get_filtered_reference_vectors(count_gen)...);
  }
};

template<typename T, size_t N = number_of_event_types, typename = std::make_index_sequence<N> >
struct any_event_type_range;

template<typename T, size_t N, size_t... event_index>
struct any_event_type_range<T, N, std::index_sequence<event_index...> > :
  std::variant<decltype(cartesian_multiproduct<event_index>().event_range())...> {};

template <size_t N = number_of_event_types,
          typename = std::make_index_sequence<N>>
struct single_time_event_generator;

template <size_t N, size_t... event_index>
struct single_time_event_generator<N, std::index_sequence<event_index...>> {

  std::tuple<cartesian_multiproduct<event_index>...> product_thing;

  constexpr const static auto concat_lambda = [](auto &...x) {
    return (ranges::views::concat(x.event_range()...));
  };

  std::variant<decltype(std::get<event_index>(product_thing).event_range())...>
      range_variant;
  /*
    cppcoro::generator<any_sir_event> test(){
    for(any_sir_event elem : std::get<0>(product_thing).event_range()){
    co_yield elem;
    }
    }
  */

  auto event_range() { return (std::apply(concat_lambda, product_thing)); }

  single_time_event_generator() = default;
  single_time_event_generator(sir_state &initial_conditions)
      : product_thing(
            cartesian_multiproduct<event_index>(initial_conditions)...){}
};

#endif

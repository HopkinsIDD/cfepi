#include <iostream>
#include <vector>
#include <variant>
#include <string>


#include <sir.h>
/*
 *
 */
#ifndef __SIR_GENERATORS_H_
#define __SIR_GENERATORS_H_

/*******************************************************************************
 * Functions to compute different things about SIR events/states               *
 *******************************************************************************/
// See sir.h
template<typename T,size_t i>
using repeat = T;

void update_from_descendent(sir_state &ours, const sir_state &theirs) {
  if (theirs.time > ours.time) {
    ours.time = theirs.time;
    for (person_t person = 0; person < ours.population_size; ++person) {
      for (size_t compartment = 0; compartment < ncompartments; ++compartment) {
        ours.potential_states[person][compartment] = false;
      }
    }
  }
  for (person_t person = 0; person < ours.population_size; ++person) {
    for (size_t compartment = 0; compartment < ncompartments; ++compartment) {
      ours.potential_states[person][compartment] =
          ours.potential_states[person][compartment] ||
          theirs.potential_states[person][compartment];
    }
  }
}

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
const auto transform_lambda = [](auto&& x){
  sir_event_by_index<event_index> rc;
  std::apply([&rc](auto... y){rc.affected_people = {y...};}, x);
  return((any_sir_event) rc);
 };

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
    return(ranges::transform_view(cartesian_range(), transform_lambda<event_index>));
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


/*******************************************************************************
 * Actual event generators                                                     *
 *******************************************************************************/
// See generators.h + sir.h

struct generator_with_sir_state : virtual public generator_with_buffered_state<sir_state, any_sir_event> {
  epidemic_time_t t_current;
  epidemic_time_t t_max;
  bool ready_to_update_from_downstream = false;
  bool check_preconditions(const sir_state& pre_state, any_sir_event event){
    auto rc = true;
    auto tmp = false;
    size_t event_size = std::visit(any_sir_event_size{},event);
    for(person_t i = 0; i < event_size; ++i){
      tmp = false;
      for(size_t compartment = 0; compartment < ncompartments; ++compartment){
	auto lhs = std::visit(any_sir_event_preconditions{i},event)[compartment];
	auto rhs = pre_state.potential_states[std::visit(any_sir_event_affected_people{i},event)][compartment];
	tmp = tmp || (lhs && rhs);
      }
      rc = rc && tmp;
    }
    return(rc);
  }
  void apply_event_to_state(sir_state& post_state,const any_sir_event& event){
    epidemic_time_t event_time = std::visit(any_sir_event_time{},event);
    post_state.time = event_time;
    size_t event_size = std::visit(any_sir_event_size{},event);
    if(event_size == 0){
      return;
    }
    size_t i = 0;
    auto to_state = std::visit(any_sir_event_postconditions{i},event);
    person_t affected_person = 0;

    for(i = 0; i < event_size; ++i){
      to_state = std::visit(any_sir_event_postconditions{i},event);
      if(to_state){
	affected_person = std::visit(any_sir_event_affected_people{i},event);
	// bool previously_in_compartment = false;
	// bool any_changes = false;
	if(!post_state.states_modified[affected_person]){
	  for(size_t previous_compartment = 0; previous_compartment < ncompartments; ++previous_compartment){
	    post_state.potential_states[affected_person][previous_compartment] = false;
	    // previously_in_compartment = std::visit(any_sir_event_preconditions{i},event)[previous_compartment];
	    // post_state.potential_states[affected_person][previous_compartment] =
	    //   previously_in_compartment ?
	    //   false :
	    //   post_state.potential_states[affected_person][previous_compartment];
	  }
	  post_state.states_modified[affected_person] = true;
	}
	post_state.potential_states[affected_person][to_state.value() ] = true;
      }
    }
  }
  void update_state(sir_state& post_state, const any_sir_event& event, const sir_state& pre_state){
    // Check preconditions
    bool preconditions_satisfied = check_preconditions(pre_state, event);
    if(preconditions_satisfied){
      apply_event_to_state(post_state,event);
    }
  }
  virtual void initialize() override {
    generator_with_buffered_state<sir_state,any_sir_event>::initialize();
  }
  bool should_update_current_state(const any_sir_event& e){
    if(any_downstream_with_state){
      if(ready_to_update_from_downstream){
	ready_to_update_from_downstream = false;
	return(true);
      }
      return(false);
      // epidemic_time_t event_time = std::visit(any_sir_event_time{},e);
      // auto rc = event_time > current_state.time;
      // return(rc);
    }
    size_t event_size = std::visit(any_sir_event_size{},e);
    return(event_size == 0);
  }
  void apply_event_to_future_state(const any_sir_event& event) override {
    update_state(future_state,event,current_state);
  }
  void update_state_from_state(sir_state& ours, const sir_state& theirs, __attribute__((unused)) std::string our_name, __attribute__((unused)) std::string their_name){
    if(ours.time < theirs.time){
      for(person_t person = 0 ; person < ours.population_size; ++person){
	for(size_t compartment = 0; compartment < ncompartments; ++compartment){
	  ours.potential_states[person][compartment] = false;
	}
      }
    }
    ours.time = theirs.time;
    for(person_t person = 0 ; person < ours.population_size; ++person){
      for(size_t compartment = 0; compartment < ncompartments; ++compartment){
	ours.potential_states[person][compartment] = ours.potential_states[person][compartment] || theirs.potential_states[person][compartment];
      }
    }
  }
  void update_state_from_buffer() override {
    current_state = future_state;
    for(size_t i = 0 ; i < future_state.potential_states.size(); ++i){
      for(size_t j = 0; j < future_state.potential_states[0].size(); ++j){
	// future_state.potential_states[i][j] = false;
      }
    }
    for(auto it = std::begin(future_state.states_modified); it != std::end(future_state.states_modified); ++it){
      (*it) = false;
    }
  }
  void update_state_from_downstream(const generator_with_buffered_state<sir_state,any_sir_event>* downstream) override {
    update_state_from_state(future_state,downstream->current_state,name,downstream->name);
  }
  generator_with_sir_state() = default;
};
/*
 * Discrete time emitting generator.  Emits events based on a local state which it updates.
 */

struct toy_generator {
  std::string name;
  std::vector<std::vector<person_t> > persons_by_precondition;
  std::vector<std::vector<person_t>::iterator > iterator_by_precondition;
  size_t event_type_index;
  bool post_events_generated;
  sir_event_constructor<number_of_event_types> event_constructor;
  single_time_event_generator<number_of_event_types> event_generator;
  decltype(event_generator.event_range()) event_range;
  decltype(ranges::begin(event_range)) event_iterator;
  sir_state initial_state;
  sir_state current_state;
  sir_state future_state;
  epidemic_time_t t_current;
  epidemic_time_t t_max;
  using event_type = any_sir_event;

  void initialize(){
    t_current = 0;
    current_state = initial_state;
    future_state = initial_state;
    update_iterators_for_new_event();
  }

  void finalize(){
    std::cout << "Finished" << "\n";
  }

  bool more_events(){
    return(event_iterator != ranges::end(event_range));
  }

  auto next_event(){

    auto rc = *event_iterator;
    std::visit(any_sir_event_set_time{t_current},rc);

    event_iterator = ranges::next(event_iterator, 1, ranges::end(event_range));

    if (ranges::end(event_range) == event_iterator) {
      if(t_current < t_max){
	++t_current;
	update_iterators_for_new_event();
      }
    }
    return(rc);
  }

  void update_iterators_for_new_event(){
    event_generator = single_time_event_generator<number_of_event_types>(current_state);
    event_range = event_generator.event_range();
    event_iterator = ranges::begin(event_range);
  }

  toy_generator(sir_state _initial_state, epidemic_time_t max_time, __attribute__((unused)) std::string _name):
    name(_name),
    initial_state(_initial_state),
    current_state(_initial_state),
    future_state(_initial_state),
    t_max(max_time) {
  }
};

struct discrete_time_generator : public generator_with_sir_state {
public:
  std::vector<std::vector<person_t> > persons_by_precondition;
  std::vector<std::vector<person_t>::iterator > iterator_by_precondition;
  epidemic_time_t t_current;
  epidemic_time_t t_max;
  size_t event_type_index;
  bool post_events_generated;
  sir_event_constructor<number_of_event_types> event_constructor;
  single_time_event_generator<number_of_event_types> event_generator;
  decltype(event_generator.event_range()) event_range;
  decltype(ranges::begin(event_range)) event_iterator;

  any_sir_event next_event() override {

    auto rc = (*event_iterator);
    event_iterator = ranges::next(event_iterator, 1, ranges::end(event_range));
    std::visit(any_sir_event_set_time{t_current},rc);
    printing_mutex.lock();
    print(rc, "next event : " );
    printing_mutex.unlock();
    if (ranges::end(event_range) == event_iterator) {
      update_state_from_all_downstream();
      ++t_current;
      update_iterators_for_new_event();
    }

    if (!any_downstream_with_state) {
      apply_event_to_future_state(rc);
    }

    return(rc);

    // Old

    if(std::visit(any_sir_event_size{},current_value) == 0){
      // If last value was a event of length 0
      // We need to update from downstream and update iterators
      ready_to_update_from_downstream = true;
      update_state_from_all_downstream();
      update_iterators_for_new_event();
    }
    // Create and populate this event
    // any_sir_event rc = event_constructor.construct_sir_event(event_type_index);


    for(person_t precondition = 0; precondition < std::visit(any_sir_event_size{},rc); ++precondition){
      auto value = (*iterator_by_precondition[precondition]);
      // std::visit([this,value](auto& x){x.affected_people[precondition] = (*iterator_by_precondition[precondition]);},rc);
      std::visit(any_sir_event_set_affected_people{precondition,value},rc);
    }

    std::visit(any_sir_event_set_time{t_current},rc);

    // Mess with iterators
    size_t this_iterator = 0;
    size_t max_iterator = std::visit(any_sir_event_size{},rc);
    bool finished = false;
    while(this_iterator < max_iterator){
      ++iterator_by_precondition[this_iterator]; // Increase the iterator at this position;
      // If this iterator is done, start it over and increment the next iterator
      /*
	// Errors here:
      if(iterator_by_precondition[this_iterator] >= std::end(persons_by_precondition[this_iterator])){
	iterator_by_precondition[this_iterator] = std::begin(persons_by_precondition[this_iterator]);
	++this_iterator;
      } else {
	// We are done incrementing the state so return
	finished = true;
	this_iterator = -1;
      }
      */

    }
    // If we reset all of the iterators, we should increment the event type
    if(!finished){
      ++event_type_index;
      while((!finished) && (event_type_index < std::variant_size<any_sir_event>::value)){
	auto any_events_of_next_type = update_iterators_for_new_event();
	// If we have at least one more event type, we are done incrementing
	if(any_events_of_next_type){
	  finished = true;
	} else {
	  ++event_type_index;
	}
      }
    }
    // Otherwise reset the event type index and we increment time
    if((!finished)){
      event_type_index = 0;
      // Wait for downstream here
      while((!finished) && (event_type_index < std::variant_size<any_sir_event>::value)){
	auto any_events_of_next_type = update_iterators_for_new_event();
	// If we have at least one more event type, we are done incrementing
	if(any_events_of_next_type){
	  finished = true;
	} else {
	  ++event_type_index;
	}
      }
      ++t_current;
      finished = true;
    }
    if(std::visit([](const auto&x){
		    bool any_person_duplicated = false;
		    for(auto it1 = std::begin(x.affected_people); it1 != std::end(x.affected_people); ++ it1){
		      for(auto it2 = it1+1; it2 != std::end(x.affected_people); ++it2){
			any_person_duplicated = any_person_duplicated || ((*it1) == (*it2));
		      }
		    }
		    return(any_person_duplicated);
		  }, rc)
      ){

      return(next_event());
    }

    return(rc);
  }

  bool update_iterators_for_new_event(){

    event_generator = single_time_event_generator<number_of_event_types>(current_state);
    event_range = event_generator.event_range();
    event_iterator = ranges::begin(event_range);

    return(true);

  }

  bool more_events() override {
    return(t_current <= t_max);
  }

  void initialize() override {
    post_events_generated = false;

    generator_with_sir_state::initialize();
    event_type_index = 0;
    t_current = 1;
    current_state = initial_state;
    future_state = initial_state;
    current_state.time = 0;
    future_state.time = 0;
    event_generator = single_time_event_generator<number_of_event_types>(current_state);
    update_iterators_for_new_event();
  }

  discrete_time_generator(sir_state _initial_state, epidemic_time_t max_time, std::string _name = "discrete time generator : "):
    generator<any_sir_event>(_name),
    generator_with_buffered_state<sir_state,any_sir_event>(_initial_state),
    generator_with_sir_state(),
    t_max(max_time) {
  }

};

struct sir_filtered_generator :
  virtual public generator_with_sir_state,
  virtual public filtered_generator<any_sir_event> {
public:
  using filtered_generator<any_sir_event>::next_event;
  using generator<any_sir_event>::name;
  any_sir_event next_event() override {
    return(filtered_generator<any_sir_event>::next_event());
  }
  bool more_events() override {
    return(filtered_generator<any_sir_event>::more_events());
  }
  std::function<bool(const sir_state&, const any_sir_event&)> user_filter;
  std::function<void(sir_state&, const any_sir_event&)> user_process;
  bool filter(const any_sir_event& event) override {
    if(check_preconditions(current_state,event)){
      return(user_filter(current_state, event));
    }
    return(false);
  }
  void process(const any_sir_event& event) override {
    return(filtered_generator<any_sir_event>::process(event));
  }
  void generate(){
    return(filtered_generator<any_sir_event>::generate());
  }
  void initialize() override {
    generator_with_buffered_state::initialize();
    filtered_generator<any_sir_event>::initialize();
    t_current = 1;
    current_state.time = 0;
    future_state.time = 0;
  }
  sir_filtered_generator(
			 generator<any_sir_event>* _parent,
			 sir_state initial_state,
			 std::function<bool(const sir_state&, const any_sir_event&)> _filter,
			 std::string _name = "sir_filtered_generator"
			 ) :
    generator<any_sir_event>(_name),
    generator_with_buffered_state<sir_state,any_sir_event>(initial_state),
    generator_with_sir_state(),
    filtered_generator<any_sir_event>(_parent),
    user_filter(_filter)
  {
  }

};

template<typename generator_t>
requires requires(generator_t gen){ { gen.more_events() } -> std::same_as<bool>; }
bool more_events(generator_t& gen){
  return(gen.more_events());
}

template<typename generator_t>
requires requires(generator_t gen){ { gen.next_event() }; }
void emit_event(generator_t& gen){
  print(gen.next_event());
}

template<typename T>
requires requires(T t){ {t.initialize()}; }
auto initialize(T& t){
  return(t.initialize());
}

template<typename T>
requires requires(T t){ {t.finalize()}; }
auto finalize(T& t){
  return(t.finalize());
}
#endif

#include <iostream>
#include <vector>
#include <variant>
#include <string>

#include "sir.hpp"
#include "generators.hpp"
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

void apply_to_postconditions(sir_state& post_state,const any_sir_event& event) {
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
      if(!post_state.states_modified[affected_person]){
	for(auto previous_compartment = 0UL ; previous_compartment < ncompartments; ++previous_compartment){
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
      std::visit(any_sir_event_apply_to_sir_state{post_state}, event);
      // update_state(post_state,event);
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

struct discrete_time_simple_generator {
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
    current_state.time = t_current;
    future_state = current_state;
    future_state.time = t_current;
    print(current_state, "current : ");
    print(future_state, "future : ");
    update_iterators_for_new_event();
  }

  void finalize(){
    update_iterators_for_new_event();
    std::cout << "Finished" << "\n";
  }

  bool more_events(){
    return(event_iterator != ranges::end(event_range));
  }

  void process(any_sir_event e){
    std::visit(any_sir_event_apply_to_sir_state{future_state}, e);
  }

  auto next_event(){

    auto rc = *event_iterator;
    std::visit(any_sir_event_set_time{t_current},rc);

    process(rc);

    event_iterator = ranges::next(event_iterator, 1, ranges::end(event_range));

    if (ranges::end(event_range) == event_iterator) {
      if(t_current < t_max){
	update_iterators_for_new_event();
      }
    }
    return(rc);
  }

  void update_current_state_from_future_state(){
    print(current_state);
    auto population_size = current_state.potential_states.size();
    // This doesn't really work in the case of actual potential states (just real states)
    for(auto person_index : ranges::views::iota( 0UL ) | ranges::views::take(population_size)){
      bool any_set = false;
      for(auto this_set : future_state.potential_states[person_index]){
	any_set = any_set || this_set;
      }
      if(!any_set){
	future_state.potential_states[person_index] = current_state.potential_states[person_index];
      }
    }
    print(future_state);
    current_state = future_state;
  }

  void update_iterators_for_new_event(){
    update_current_state_from_future_state();
    ++t_current;
    event_generator = single_time_event_generator<number_of_event_types>(current_state);
    event_range = event_generator.event_range();
    event_iterator = ranges::begin(event_range);
    for(auto& potentials : future_state.potential_states){
      for(auto& value : potentials){
	value = false;
      }
    }
  }

  discrete_time_simple_generator(sir_state _initial_state, epidemic_time_t max_time, std::string _name):
    name(_name),
    initial_state(_initial_state),
    current_state(_initial_state),
    future_state(_initial_state),
    t_max(max_time) {
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
  auto current_event = gen.next_event();
  print(current_event);
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

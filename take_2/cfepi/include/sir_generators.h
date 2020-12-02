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


struct generator_with_sir_state : virtual public generator_with_buffered_state<sir_state, any_sir_event> {
  sir_state initial_state;
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
  void apply_to_postconditions(sir_state& post_state,const any_sir_event& event){
    epidemic_time_t event_time = std::visit(any_sir_event_time{},event);
    if(!(post_state.time == event_time)){
      printing_mutex.lock();
      std::cout << name << "Updating time from " << post_state.time << " to " << event_time << std::endl;
      printing_mutex.unlock();
    }
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
	bool previously_in_compartment = false;
	bool any_changes = false;
	if(!post_state.states_modified[affected_person]){
	  for(auto previous_compartment = 0; previous_compartment < ncompartments; ++previous_compartment){
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
      if(DEBUG_EVENT_PRINT){
	std::string msg = "postconditions satisfied, applying event";
	printing_mutex.lock();
	debug_print(msg,name);
	printing_mutex.unlock();
      }
      apply_to_postconditions(post_state,event);
    }
  };
  virtual void initialize(){
    if(DEBUG_PRINT){
      std::string msg = "initialize as generator_with_sir_state";
      printing_mutex.lock();
      debug_print(msg,name);
      printing_mutex.unlock();
    }
    generator_with_buffered_state<sir_state,any_sir_event>::initialize();
    current_state = initial_state;
    future_state = initial_state;
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
  };
  void apply_event_to_future_state(const any_sir_event& event){
    update_state(future_state,event,current_state);
  };
  void update_state_from_state(sir_state& ours, const sir_state& theirs, std::string our_name, std::string their_name){
    if(ours.population_size != theirs.population_size){
      printing_mutex.lock();
      std::cout << our_name << "Population sizes differ" << std::endl;
      printing_mutex.unlock();
      throw Exception();
    }
    printing_mutex.lock();
    std::cout << our_name << "Updating from " << their_name << std::endl;
    std::cout << our_name << "  our time " << ours.time << std::endl;
    std::cout << our_name << "  their time " << theirs.time << std::endl;
    printing_mutex.unlock();
    if(ours.time < theirs.time){
      for(person_t person = 0 ; person < ours.population_size; ++person){
	for(auto compartment = 0; compartment < ncompartments; ++compartment){
	  ours.potential_states[person][compartment] = false;
	}
      }
    }
    ours.time = theirs.time;
    for(person_t person = 0 ; person < ours.population_size; ++person){
      for(auto compartment = 0; compartment < ncompartments; ++compartment){
	ours.potential_states[person][compartment] = ours.potential_states[person][compartment] || theirs.potential_states[person][compartment];
      }
    }
  }
  void update_state_from_buffer(){
    printing_mutex.lock();
    std::cout << name << "Updating current state from time " << current_state.time << " to " << future_state.time << std::endl;
    printing_mutex.unlock();
    current_state = future_state;
    for(size_t i = 0 ; i < future_state.potential_states.size(); ++i){
      for(size_t j = 0; j < future_state.potential_states[0].size(); ++j){
	// future_state.potential_states[i][j] = false;
      }
    }
    for(auto it = std::begin(future_state.states_modified); it != std::end(future_state.states_modified); ++it){
      (*it) = false;
    }
    if(DEBUG_STATE_PRINT){
      printing_mutex.lock();
      print(current_state,name + " current : ");
      print(future_state,name + " future : ");
      printing_mutex.unlock();
    }
  }
  void update_state_from_downstream(const generator_with_state<sir_state,any_sir_event>* downstream){
    update_state_from_state(current_state,downstream->current_state,name,downstream->name);
  };
  generator_with_sir_state(sir_state initial_state, std::string _name) :
    generator<any_sir_event>(_name),
    generator_with_state<sir_state, any_sir_event>(initial_state,_name),
    generator_with_buffered_state<sir_state,any_sir_event>(initial_state,_name),
    initial_state(initial_state) {
  };
};
/*
 * Discrete time emitting generator.  Emits events based on a local state which it updates.
 */

struct discrete_time_generator : public generator_with_sir_state {
public:
  std::vector<std::vector<person_t> > persons_by_precondition;
  std::vector<std::vector<person_t>::iterator > iterator_by_precondition;
  epidemic_time_t t_current;
  epidemic_time_t t_max;
  size_t event_type_index;
  bool post_events_generated;
  any_sir_event next_event(){
    if(std::visit(any_sir_event_size{},current_value) == 0){
      // If last value was a event of length 0
      // We need to update from downstream and update iterators
      if(DEBUG_PRINT){
	printing_mutex.lock();
	std::cout << name << "Starting new time, so updating state from downstream" << std::endl;
	printing_mutex.unlock();
      }
      ready_to_update_from_downstream = true;
      update_state_from_all_downstream(current_value);
      update_iterators_for_new_event();
      if(DEBUG_PRINT){
	printing_mutex.lock();
	print(current_state, name + " current state : ");
	printing_mutex.unlock();
      }
    }
    // Create and populate this event
    any_sir_event rc = construct_sir_by_event_index(event_type_index);


    for(person_t precondition = 0; precondition < std::visit(any_sir_event_size{},rc); ++precondition){
      auto value = (*iterator_by_precondition[precondition]);
      // std::visit([this,value](auto& x){x.affected_people[precondition] = (*iterator_by_precondition[precondition]);},rc);
      std::visit(any_sir_event_set_affected_people{precondition,value},rc);
    }

    std::visit(any_sir_event_set_time{t_current},rc);

    if(DEBUG_EVENT_PRINT){
      printing_mutex.lock();
      std::string msg = "Preparing to generate event";
      debug_print(msg,name);
      debug_print(rc,name);
      printing_mutex.unlock();
    }

    if(event_type_index == (number_of_event_types - 1)){
      printing_mutex.lock();
      std::cout << name << "finishing with time " << t_current << std::endl;
      printing_mutex.unlock();
    }

    if((event_type_index > 0) && (current_state.time + 1 < t_current)){
	printing_mutex.lock();
	std::string msg = "Events and state separated in time";
	std::cout << name << msg << std::endl;
	msg = "event time " + std::to_string(t_current);
	std::cout << name << msg << std::endl;
	msg = "state time " + std::to_string(current_state.time);
	std::cout << name << msg << std::endl;
	printing_mutex.unlock();
	throw Exception();
    }

    // process(rc);

    if(DEBUG_EVENT_PRINT){
      std::string msg = "Getting ready for next event";
      printing_mutex.lock();
      debug_print(msg,name);
      printing_mutex.unlock();
    }
    // Mess with iterators
    size_t this_iterator = 0;
    size_t max_iterator = std::visit(any_sir_event_size{},rc);
    bool finished = false;
    while(this_iterator < max_iterator){
      if(DEBUG_STATE_PRINT){
	std::string msg = "incrementing iterator " + std::to_string(this_iterator) + "/" + std::to_string(max_iterator);
	printing_mutex.lock();
	debug_print(msg,name);
	printing_mutex.unlock();
      }
      ++iterator_by_precondition[this_iterator]; // Increase the iterator at this position;
      // If this iterator is done, start it over and increment the next iterator
      if(DEBUG_STATE_PRINT){
	auto diff = iterator_by_precondition[this_iterator] - std::begin(persons_by_precondition[this_iterator]);
	std::string msg = "    iterator has gone " + std::to_string(diff) + " steps";
	printing_mutex.lock();
	debug_print(msg,name);
	diff = std::end(persons_by_precondition[this_iterator]) - iterator_by_precondition[this_iterator];
	msg = "iterator has " + std::to_string(diff) + " steps left to go";
	debug_print(msg,name);
	printing_mutex.unlock();
      }
      if(iterator_by_precondition[this_iterator] >= std::end(persons_by_precondition[this_iterator])){
	if(DEBUG_PRINT){
	  std::string msg = "    reseting iterator " + std::to_string(this_iterator);
	  printing_mutex.lock();
	  debug_print(msg,name);
	  printing_mutex.unlock();
	}
	iterator_by_precondition[this_iterator] = std::begin(persons_by_precondition[this_iterator]);
	++this_iterator;
      } else {
	// We are done incrementing the state so return
	finished = true;
	this_iterator = -1;
      }

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
      // printing_mutex.lock();
      // std::cout << name << "    Should wait for downstream here" << std::endl;
      // printing_mutex.unlock();
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
  };
  bool update_iterators_for_new_event(){
    if(DEBUG_PRINT){
      printing_mutex.lock();
      std::string msg = "    moving to events of type " + std::to_string(event_type_index);
      debug_print(msg,name);
      printing_mutex.unlock();
    }

    any_sir_event rc = construct_sir_by_event_index(event_type_index);
    auto number_of_iterators = std::visit([](auto& x){return(x.preconditions.size());},rc);
    iterator_by_precondition.resize(number_of_iterators);
    persons_by_precondition.resize(number_of_iterators);
    bool added = false;
    for(size_t precondition = 0; precondition < number_of_iterators; ++precondition){
      persons_by_precondition[precondition].clear();
      persons_by_precondition[precondition].reserve(current_state.population_size);
      for (person_t p = 0; p < current_state.population_size ; ++p){
	added = false;
	for(size_t compartment = 0; compartment < ncompartments; ++compartment){
	  if((!added) &&
	     (current_state.potential_states[p][compartment]) &&
	     (std::visit([precondition,compartment](auto&x){return(x.preconditions[precondition][compartment]);},rc))) {
	    persons_by_precondition[precondition].push_back(p);
	    added = true;
	  }
	}
      }

      iterator_by_precondition[precondition] = std::begin(persons_by_precondition[precondition]);
      if(iterator_by_precondition[precondition] == std::end(persons_by_precondition[precondition])){
	return(false);
      }
    }
    return(true);
  };
  bool more_events(){
    return(t_current <= t_max);
  };
  void initialize() {
    if(DEBUG_PRINT){
      std::string msg = "initialize as discrete_time_generator";
      printing_mutex.lock();
      debug_print(msg,name);
      printing_mutex.unlock();
    }
    post_events_generated = false;

    generator_with_sir_state::initialize();
    event_type_index = 0;
    t_current = 1;
    current_state.time = 0;
    future_state.time = 1;
    auto update_success = update_iterators_for_new_event();
    if(!update_success){
      printing_mutex.lock();
      std::cout << "Initial update successful.  Possibly a problem with initial conditions" << std::endl;
      printing_mutex.unlock();
      throw Exception();
    }
  };

  discrete_time_generator(sir_state _initial_state, epidemic_time_t max_time, std::string _name = "discrete time generator : "):
    generator<any_sir_event>(_name),
    generator_with_state<sir_state,any_sir_event>(_initial_state,_name),
    generator_with_buffered_state<sir_state,any_sir_event>(_initial_state,_name),
    generator_with_sir_state(_initial_state, _name),
    t_max(max_time) {
  };
};

struct sir_filtered_generator :
  virtual public generator_with_sir_state,
  virtual public filtered_generator<any_sir_event> {
public:
  using filtered_generator<any_sir_event>::next_event;
  using generator<any_sir_event>::name;
  any_sir_event next_event(){
    return(filtered_generator<any_sir_event>::next_event());
  }
  bool more_events(){
    if(DEBUG_STATE_PRINT){
      printing_mutex.lock();
      print(current_state,name + " current state : ");
      print(future_state,name + " future state : ");
      printing_mutex.unlock();
    }
    return(filtered_generator<any_sir_event>::more_events());
  }
  std::function<bool(const sir_state&, const any_sir_event&)> user_filter;
  std::function<void(sir_state&, const any_sir_event&)> user_process;
  bool filter(const any_sir_event& event) {
    if(check_preconditions(current_state,event)){
      if(DEBUG_EVENT_PRINT){
	printing_mutex.lock();
	std::cout << name << "Event" << std::endl;
	print(event,name + "  " );
	std::cout << name << "  passes preconditions" << std::endl;
	printing_mutex.unlock();
      }
      return(user_filter(current_state, event));
    }
    return(false);
  };
  void process(const any_sir_event& event){
    generator_with_sir_state::process(event);
  };
  void generate(){
    return(filtered_generator<any_sir_event>::generate());
  }
  void initialize(){
    if(DEBUG_PRINT){
      std::string msg = "initialize as sir_filtered_generator";
      printing_mutex.lock();
      debug_print(msg,generator<any_sir_event>::name);
      printing_mutex.unlock();
    }
    generator_with_state::initialize();
    filtered_generator<any_sir_event>::initialize();
    t_current = 1;
    current_state.time = 0;
    future_state.time = 1;
  }
  sir_filtered_generator(
			 generator<any_sir_event>* _parent,
			 sir_state initial_state,
			 std::function<bool(const sir_state&, const any_sir_event&)> _filter,
			 std::string _name = "sir_filtered_generator"
			 ) :
    generator<any_sir_event>(_name),
    generator_with_state<sir_state,any_sir_event>(initial_state,_name),
    generator_with_buffered_state<sir_state,any_sir_event>(initial_state,_name),
    generator_with_sir_state(initial_state,_name),
    filtered_generator<any_sir_event>(_parent,_name),
    user_filter(_filter)
  {
  };

};

#endif

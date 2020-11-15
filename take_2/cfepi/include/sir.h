// this_thread::sleep_for example
#include <iostream>       // std::cout, std::endl
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds
// this_thread::yield example
#include <atomic>         // std::atomic
#include <functional>
#include <typeinfo>
#include <mutex>
#include <generators.h>
#include <vector>
#include <variant>
#include <queue>
// #include <stxxl/vector>

#ifndef __SIR_H_
#define __SIR_H_


enum epidemic_state {S, I, R, ncompartments};
typedef float epidemic_time_t;
typedef size_t person_t;
size_t global_event_counter = 0;

/*
 * @name State
 * @description The state of the model.
 */

struct sir_state {
  person_t population_size = 0;
  std::vector<std::array<bool,ncompartments> > potential_states;
  std::vector<bool> states_modified;
  epidemic_time_t time = -1;
  float beta = .2;
  float gamma = .1;
  std::string prefix;
  sir_state() noexcept = default;
  sir_state(person_t population_size = 10) noexcept : population_size(population_size){
    potential_states.resize(population_size);
    states_modified.resize(population_size);
  };
};


/*
 * @name SIR Event
 * @description An event that transitions the model between states
 */

template<size_t size>
struct sir_event {
  epidemic_time_t time;
  std::array<person_t,size> affected_people;
  std::array<std::array<bool,ncompartments>,size> preconditions;
  std::array<std::optional<epidemic_state>,size> postconditions;
  sir_event() noexcept = default;
  sir_event(const sir_event&) = default;
};

struct infection_event : public sir_event<2> {
  using sir_event<2>::time;
  using sir_event<2>::affected_people;
  using sir_event<2>::preconditions;
  using sir_event<2>::postconditions;
  infection_event() noexcept {
    preconditions = {std::array<bool,3>({true,false,false}),std::array<bool,3>({false,true,false})};
    postconditions[0].emplace(I);
  };
  infection_event(const infection_event&) = default;
  infection_event(person_t p1, person_t p2, epidemic_time_t _time) {
    time = _time;
    affected_people = {p1, p2};
    preconditions = {std::array<bool,3>({true,false,false}),std::array<bool,3>({false,true,false})};
    postconditions[0].emplace(I);
  }
};

struct recovery_event : public sir_event<1> {
  using sir_event<1>::time;
  using sir_event<1>::affected_people;
  using sir_event<1>::preconditions;
  using sir_event<1>::postconditions;
  recovery_event() noexcept {
    preconditions = {std::array<bool,3>({false,true,false})};
    postconditions[0].emplace(R);
  };
  recovery_event(const recovery_event&) = default;
  recovery_event(person_t p1, epidemic_time_t _time) {
    time = _time;
    affected_people[0] = p1;
    preconditions = {std::array<bool,3>({false,true,false})};
    postconditions[0].emplace(R);
  }
};

struct null_event : public sir_event<0> {
  using sir_event<0>::time;
  using sir_event<0>::affected_people;
  using sir_event<0>::preconditions;
  using sir_event<0>::postconditions;
  null_event() noexcept = default;
  null_event(const null_event&) = default;
  null_event(epidemic_time_t _time) {
    time = _time;
  }
};
typedef std::variant < null_event, recovery_event, infection_event > any_sir_event;

any_sir_event construct_sir_by_event_index(size_t index){
  switch(index) {
  case 0: return(null_event());
  case 1: return(recovery_event());
  case 2: return(infection_event());
  }
  std::cout << "Bad index " << index << std::endl;
  throw Exception();
}

/*
 * Printing helpers
 */
void print(const any_sir_event& event, std::string prefix = ""){
  printing_mutex.lock();
  std::cout << prefix;
  std::visit([](auto& event){
    std::cout << "time " << event.time << " ";
    for(auto it : event.affected_people){
      std::cout << it << ", ";
    }
    std::cout << std::endl;
  },event);
  printing_mutex.unlock();
};

void print(const sir_state& state, std::string prefix = ""){
  printing_mutex.lock();
  std::cout << prefix << "Possible states at time " << state.time << std::endl;
  int person_counter = 0;
  int state_count = ncompartments;
  for(auto possible_states : state.potential_states){
    std::cout << prefix << person_counter << "(";
    state_count = 0;
    for(auto state : possible_states){
      if(state){
	std::cout << " " << state_count;
      }
      ++state_count;
    }
    std::cout << " )" << std::endl;
    ++person_counter;
  }
  printing_mutex.unlock();
};

auto time_compare = [](auto& x, auto&y){
  return(
	 std::visit(
		    [](auto &new_x, auto&new_y){
		      return(new_x.time > new_y.time);
		    },
		    x,
		    y
		    )
	 );
 };

auto should_update_current_state = [](const sir_state& state, const any_sir_event& event){
  epidemic_time_t event_time = std::visit([](auto x){return(x.time);},event);
  epidemic_time_t state_time = state.time;
  bool rc = state_time < event_time;
  return(rc);
 };

auto update_state = [](sir_state& post_state, const any_sir_event& event, const sir_state& pre_state){
  auto apply_postconditions = [&post_state](auto& x){
    for(size_t i = 0; i < x.affected_people.size(); ++i){

    }
    for(size_t i = 0; i < x.affected_people.size(); ++i){
      if(x.postconditions[i]){
	if(DEBUG_PRINT){
	  printing_mutex.lock();
	  std::cout << "changing person " << x.affected_people[i] << " to state " << x.postconditions[i].value() << std::endl;
	  printing_mutex.unlock();
	}
	if(!post_state.states_modified[x.affected_people[i]]){
	  if(DEBUG_PRINT){
	    printing_mutex.lock();
	    std::cout << "first change of person " << x.affected_people[i] << " this time" << std::endl;
	    printing_mutex.unlock();
	  }
	  for(auto previous_compartment = 0; previous_compartment < ncompartments; ++previous_compartment){
	    post_state.potential_states[x.affected_people[i] ][previous_compartment] = x.preconditions[i][previous_compartment] ? false : post_state.potential_states[x.affected_people[i] ][previous_compartment];
	  }
	  post_state.states_modified[x.affected_people[i] ] = true;
	}
	post_state.potential_states[x.affected_people[i] ][x.postconditions[i].value() ] = true;
	post_state.time = x.time;
      }
    }
  };
  std::visit(apply_postconditions, event);
 };

auto update_from_descendent = [](sir_state& ours, const sir_state& theirs){
  if(ours.population_size != theirs.population_size){
    printing_mutex.lock();
    std::cout << "Population sizes differ" << std::endl;
    printing_mutex.unlock();
    throw Exception();
  }
  if(theirs.time > ours.time){
    ours.time = theirs.time;
    for(person_t person = 0 ; person < ours.population_size; ++person){
      for(auto compartment = 0; compartment < ncompartments; ++compartment){
	ours.potential_states[person][compartment] = false;
      }
    }
  }
  for(person_t person = 0 ; person < ours.population_size; ++person){
    for(auto compartment = 0; compartment < ncompartments; ++compartment){
      ours.potential_states[person][compartment] |= theirs.potential_states[person][compartment];
    }
  }
 };

sir_state default_state(person_t popsize=10){
  sir_state rc(popsize);
  rc.potential_states[0][I] = true;
  for(size_t i = 0 ; i < rc.potential_states.size(); ++i){
    rc.potential_states[i][S] = true;
  }
  return(rc);
}

/*
 *
 */
struct generator_with_sir_state : public generator_with_buffered_state<sir_state, any_sir_event> {
  sir_state initial_state;
  epidemic_time_t t_current;
  epidemic_time_t t_max;
  virtual void initialize(){
    if(DEBUG_PRINT){
      std::string msg = "initialize as generator_with_sir_state";
      debug_print(msg,name);
    }
    generator_with_buffered_state<sir_state,any_sir_event>::initialize();
    current_state = initial_state;
    future_state = initial_state;
  }
  bool should_update_current_state(const any_sir_event& e){
    epidemic_time_t event_time = std::visit([](auto&x){return(x.time);},e);
    return(event_time > current_state.time);
  };
  void apply_event_to_future_state(const any_sir_event& event){
    update_state(future_state,event,current_state);
  };
  void update_state_from_buffer(){
    current_state = future_state;
    update_from_descendent(future_state,initial_state);
    for(auto it = std::begin(future_state.states_modified); it != std::end(future_state.states_modified); ++it){
      (*it) = false;
    }
  }
  void update_state_from_downstream(const generator_with_state<sir_state,any_sir_event>* downstream){
    update_from_descendent(current_state,downstream->current_state);
  };
  generator_with_sir_state(sir_state initial_state, std::string _name) :
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
  size_t number_of_event_types = 2;
  any_sir_event next_event(){
    if(DEBUG_PRINT){
      ++global_event_counter;
      std::string msg = "generating next event " + std::to_string(global_event_counter);
      debug_print(msg,name);
    }

    // Create and populate this event
    any_sir_event rc = construct_sir_by_event_index(event_type_index);
    if(DEBUG_PRINT){
      std::string msg = "  event of type " + std::to_string(event_type_index);
      debug_print(msg,name);
    }

    for(person_t precondition = 0; precondition < std::visit([](auto& x){return(x.affected_people.size());},rc); ++precondition){
      std::visit([this,precondition](auto& x){x.affected_people[precondition] = (*iterator_by_precondition[precondition]);},rc);
      if(DEBUG_PRINT){
	auto tmp = std::visit([precondition](auto&x){return(x.affected_people[precondition]);}, rc);
	std::string msg = "    involving person " + std::to_string(tmp);
	debug_print(msg,name);
      }
    }
    std::visit([this](auto& x){x.time = t_current;},rc);

    process(rc);

    // Mess with iterators
    size_t this_iterator = 0;
    size_t max_iterator = std::visit([](auto&x){
				   std::cout << x.preconditions.size();
				   return(x.affected_people.size());
				 },rc); // Start at the last iterator
    bool finished = false;
    while(this_iterator < max_iterator){
      if(DEBUG_PRINT){
	std::string msg = "incrementing iterator " + std::to_string(this_iterator) + "/" + std::to_string(max_iterator);
	debug_print(msg,name);
      }
      ++iterator_by_precondition[this_iterator]; // Increase the iterator at this position;
      // If this iterator is done, start it over and increment the next iterator
      if(DEBUG_PRINT){
	auto diff = iterator_by_precondition[this_iterator] - std::begin(persons_by_precondition[this_iterator]);
	std::string msg = "iterator has gone " + std::to_string(diff) + " steps";
	debug_print(msg,name);
	diff = std::end(persons_by_precondition[this_iterator]) - iterator_by_precondition[this_iterator];
	msg = "iterator has " + std::to_string(diff) + " steps left to go";
	debug_print(msg,name);
      }
      if(iterator_by_precondition[this_iterator] >= std::end(persons_by_precondition[this_iterator])){
	if(DEBUG_PRINT){
	  std::string msg = "    reseting iterator " + std::to_string(this_iterator);
	  debug_print(msg,name);
	}
	iterator_by_precondition[this_iterator] = std::begin(persons_by_precondition[this_iterator]);
	++this_iterator;
	std::cout << this_iterator << "    new iterator" << std::endl;
      } else {
	// We are done incrementing the state so return
	finished = true;
	this_iterator = -1;
      }

    }
    // If we reset all of the iterators, we should increment the event type
    if(!finished){
      ++event_type_index;
      if(event_type_index < std::variant_size<any_sir_event>::value){
	update_iterators_for_new_event();
	// If we have at least one more event type, we are done incrementing
	finished = true;
      }
    }
    // Otherwise reset the event type index and we increment time
    if((!finished)){
      event_type_index = 0;
      update_iterators_for_new_event();
      ++t_current;
      finished = true;
    }
    if(std::visit([](auto&x){
		    bool same_person = false;
		    for(auto it1 = std::begin(x.affected_people); it1 != std::end(x.affected_people); ++ it1){
		      for(auto it2 = it1+1; it2 != std::end(x.affected_people); ++it2){
			same_person = same_person || ((*it1) == (*it2));
		      }
		    }
		    return(same_person);
		  }, rc)
      ){

      return(next_event());
    }

    if(global_event_counter >= 20){
      debug_print(current_state,name);
      debug_print(future_state,name);
    }

    return(rc);
  };
  void update_iterators_for_new_event(){
    if(DEBUG_PRINT){
      std::string msg = "moving to events of type " + std::to_string(event_type_index);
      debug_print(msg,name);
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
	    if(DEBUG_PRINT){
	      std::string msg = "adding person to precondition " + std::to_string(precondition);
	      debug_print(msg,name);
	    }
	    persons_by_precondition[precondition].push_back(p);
	    added = true;
	  }
	}
      }

      iterator_by_precondition[precondition] = std::begin(persons_by_precondition[precondition]);
      if(iterator_by_precondition[precondition] == std::end(persons_by_precondition[precondition])){
	printing_mutex.lock();
	std::cout << "persons_by_precondition is of size 0" << std::endl;
	printing_mutex.unlock();
      }
    }
  };
  bool more_events(){
    return(t_current <= t_max);
  };
  void initialize() {
    if(DEBUG_PRINT){
      std::string msg = "initialize as discrete_time_generator";
      debug_print(msg,name);
    }

    generator_with_sir_state::initialize();
    event_type_index = 0;
    t_current = 0;
    current_state.time = 0;
    future_state.time = 0;
    update_iterators_for_new_event();
  };

  discrete_time_generator(sir_state _initial_state, epidemic_time_t max_time, std::string _name = "discrete time generator : "):
    generator_with_sir_state(_initial_state, _name), t_max(max_time) {
  };
};

auto do_nothing = [](auto&x, auto&y){return;};

struct sir_filtered_generator :
  public generator_with_sir_state,
  public filtered_generator<any_sir_event> {
public:
  using filtered_generator<any_sir_event>::next_event;
  any_sir_event next_event(){
    return(filtered_generator<any_sir_event>::next_event());
  }
  bool more_events(){
    return(filtered_generator<any_sir_event>::more_events());
  }
  std::function<bool(const sir_state&, const any_sir_event&)> user_filter;
  std::function<void(sir_state&, const any_sir_event&)> user_process;
  bool filter(const any_sir_event& event) {
    return(user_filter(current_state, event));
  };
  void process(const any_sir_event& event){
    generator_with_state<sir_state,any_sir_event>::process(event);
  };
  void generate(){
    return(generator_with_state::generate());
  }
  void initialize(){
    if(DEBUG_PRINT){
      std::string msg = "initialize as sir_filtered_generator";
      debug_print(msg,generator_with_sir_state::name);
    }
    generator_with_state::initialize();
    filtered_generator<any_sir_event>::initialize();
  }
  sir_filtered_generator(
			 generator<any_sir_event>* _parent,
			 sir_state initial_state,
			 std::function<bool(const sir_state&, const any_sir_event&)> _filter,
			 std::string _name = "sir_filtered_generator"
			 ) :
    generator_with_sir_state(initial_state,_name),
    filtered_generator<any_sir_event>(_parent,_name),
    user_filter(_filter)
  {
  };

};
/*
 * Gillespie style emitting generator.  Uses a priority queue to emit events as their time threshold comes
 */

// typedef stxxl::VECTOR_GENERATOR<any_sir_event>::result any_sir_event_vector;
typedef std::vector<any_sir_event> any_sir_event_vector;

/*
struct gillespie_generator : generator<any_sir_event> {
  using generator<any_sir_event >::running;
  person_t popsize = 10;
  epidemic_time_t tmax = 40;
  void generate(){
    running = false;
    epidemic_time_t current_time = 0;
    std::priority_queue<
      any_sir_event,
      any_sir_event_vector,
      // std::vector<any_sir_event>,
      decltype(time_compare)
      > upcoming_events(time_compare);
    epidemic_time_t delta_time;
    for (person_t p1 = 0; p1 < popsize; ++p1) {
      for (person_t p2 = 0; p2 < popsize; ++p2) {
	delta_time = 1;
	upcoming_events.push(infection_event(p1,p2,current_time + delta_time));
      }
      delta_time = 1;
      upcoming_events.push(recovery_event(p1,current_time + delta_time));
    }
    current_value_mutex.lock();
    current_value = upcoming_events.top();
    delta_time = std::visit([](auto& x){return(x.affected_people[0]+1);},upcoming_events.top());
    auto tmp_val = upcoming_events.top();
    upcoming_events.pop();
    std::visit([delta_time](auto &x){x.time = x.time + delta_time;return;},tmp_val);
    upcoming_events.push(tmp_val);
    current_time = std::visit([](auto& x){return(x.time);},current_value);
    current_value_mutex.unlock();
    bool skip_next = true;
    event_counter = 0;
    running = true;
    ++event_counter;

    while (current_time < tmax) {
      while(!is_ready()){
	std::this_thread::yield();
	// std::this_thread::sleep_for(std::chrono::microseconds(1));
      }
      current_value_mutex.lock();
      current_value = upcoming_events.top();
      delta_time = std::visit([](auto& x){return(1);},upcoming_events.top());
      auto tmp_val = upcoming_events.top();
      upcoming_events.pop();
      std::visit([delta_time](auto &x){x.time = x.time + delta_time;return;},tmp_val);
      upcoming_events.push(tmp_val);
      current_time = std::visit([](auto& x){return(x.time);},current_value);
      current_value_mutex.unlock();
      ++event_counter;
    }
    while(!is_ready()){
      std::this_thread::yield();
    }
    running = false;
  };
};

struct stupid_generator : generator<any_sir_event> {
  using generator<any_sir_event >::running;
  person_t popsize = 10;
  epidemic_time_t tmax = 40;
  void generate(){
    running = false;
    current_value_mutex.lock();
    current_value_mutex.unlock();
    current_value = recovery_event(0,0);
    bool skip_next = true;
    event_counter = 0;
    running = true;
    ++event_counter;
    for (epidemic_time_t t = 0; t < tmax; ++t){
      for (person_t p1 = 0; p1 < popsize; ++p1) {
	for (person_t p2 = 0; p2 < popsize; ++p2) {
	  while(!is_ready()){
	    std::this_thread::yield();
	    // std::this_thread::sleep_for(std::chrono::microseconds(1));
	  }
	  current_value_mutex.lock();
	  current_value = infection_event(p1,p2,t);
	  current_value_mutex.unlock();
	  ++event_counter;
	}
	if(skip_next){
	  skip_next = false;
	  continue;
	}
	while(!is_ready()){
	  std::this_thread::yield();
	  // std::this_thread::sleep_for(std::chrono::microseconds(1));
	}
	current_value_mutex.lock();
	current_value = recovery_event(p1,t);
	current_value_mutex.unlock();
	++event_counter;
      }
    }
    while(!is_ready()){
      std::this_thread::yield();
    }
    running = false;
  };
};
*/


#endif

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
typedef int person;

/*
 * @name State
 * @description The state of the model.
 */

struct sir_state {
  int population_size = 0;
  std::vector<std::array<bool,ncompartments> > potential_states;
  std::vector<bool> states_modified;
  epidemic_time_t time = -1;
  float beta = .2;
  float gamma = .1;
  std::string prefix;
  sir_state(int population_size = 10) : population_size(population_size){
    potential_states.resize(population_size);
    states_modified.resize(population_size);
  };
};


/*
 * @name Event
 * @description An event that transitions the model between states
 */

template<int size>
struct sir_event {
  epidemic_time_t time;
  std::array<person,size> affected_people;
  std::array<std::array<bool,ncompartments>,size> preconditions;
  std::array<std::optional<epidemic_state>,size> postconditions;
};

struct infection_event : public sir_event<2> {
  using sir_event<2>::time;
  using sir_event<2>::affected_people;
  using sir_event<2>::preconditions;
  using sir_event<2>::postconditions;
  infection_event() noexcept = default;
  infection_event(const infection_event&) = default;
  infection_event(person p1, person p2, epidemic_time_t _time) {
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
  recovery_event() : sir_event<1>() {};
  recovery_event(person p1, epidemic_time_t _time) {
    time = _time;
    affected_people[0] = p1;
    preconditions = {std::array<bool,3>({false,true,false})};
    postconditions[0].emplace(R);
  }
};

typedef std::variant < recovery_event, infection_event > any_sir_event;

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

auto update_if_satisfied = [](sir_state& state, const any_sir_event& event){
  bool event_preconditions_satisfied = true;
  // if(DEBUG_PRINT){std::string pf = "update: "; print(pf,"test");}
  if(DEBUG_PRINT){
    print(state, "update: ");
  }
  if(DEBUG_PRINT){print(event,"update: ");}
  auto check_preconditions = [&state](auto& x){
    bool rc;
    bool tmp;
    rc = true;
    for(size_t i = 0; i < x.affected_people.size(); ++i){
      tmp = false;
      for(size_t compartment = 0; compartment < ncompartments; ++compartment){
	tmp = tmp || (x.preconditions[i][compartment] && state.potential_states[x.affected_people[i] ][compartment]);
	if(DEBUG_PRINT && (x.preconditions[i][compartment] && state.potential_states[x.affected_people[i] ][compartment])){
	  printing_mutex.lock();
	  std::cout << x.affected_people[i] << " is in state " << compartment << " satisfying precondition" << std::endl;
	  printing_mutex.unlock();
	}
      }
      rc = rc && tmp;
    }
    return(rc);
  };
  bool preconditions_satisfied = std::visit(check_preconditions, event);
  if(preconditions_satisfied){
    if(DEBUG_PRINT){
      printing_mutex.lock();
      std::cout << "all preconditions satisfied" << std::endl;
      printing_mutex.unlock();
    }
    auto apply_postconditions = [&state](auto& x){
      for(size_t i = 0; i < x.affected_people.size(); ++i){
	if(x.postconditions[i]){
	  if(DEBUG_PRINT){
	    printing_mutex.lock();
	    std::cout << "changing person " << x.affected_people[i] << " to state " << x.postconditions[i].value() << std::endl;
	    printing_mutex.unlock();
	  }
	  if(!state.states_modified[x.affected_people[i]]){
	    if(DEBUG_PRINT){
	      printing_mutex.lock();
	      std::cout << "first change of person " << x.affected_people[i] << " this time" << std::endl;
	      printing_mutex.unlock();
	    }
	    for(auto previous_compartment = 0; previous_compartment < ncompartments; ++previous_compartment){
	      state.potential_states[x.affected_people[i] ][previous_compartment] = x.preconditions[i][previous_compartment] ? false : state.potential_states[x.affected_people[i] ][previous_compartment];
	    }
	    state.states_modified[x.affected_people[i] ] = true;
	  }
	  state.potential_states[x.affected_people[i] ][x.postconditions[i].value() ] = true;
	  state.time = x.time;
	}
      }
    };
    std::visit(apply_postconditions, event);
    if(DEBUG_PRINT){
      print(state, "update: ");
    }
  }
 };

auto update_from_descendent = [](sir_state& ours, const sir_state& theirs){
  if(ours.population_size != theirs.population_size){
    throw Exception();
  }
  if(theirs.time > ours.time){
    ours.time = theirs.time;
    for(auto person = 0 ; person < ours.population_size; ++person){
      for(auto compartment = 0; compartment < ncompartments; ++compartment){
	ours.potential_states[person][compartment] = false;
      }
    }
  }
  for(auto person = 0 ; person < ours.population_size; ++person){
    for(auto compartment = 0; compartment < ncompartments; ++compartment){
      ours.potential_states[person][compartment] |= theirs.potential_states[person][compartment];
    }
  }
 };

sir_state default_state(int popsize=10){
  sir_state rc(popsize);
  rc.potential_states[0][I] = true;
  for(size_t i = 0 ; i < rc.potential_states.size(); ++i){
    rc.potential_states[i][S] = true;
  }
  return(rc);
}

/*
 * Discrete time emitting generator.  Emits events based on a local state which it updates.
 */
struct discrete_time_generator : public generator_with_polled_state<sir_state, any_sir_event> {
  //struct discrete_time_generator : public generator_with_state<sir_state, any_sir_event> {
public:
  // using generator<any_sir_event >::running;
  // using generator_with_state<sir_state, any_sir_event >::current_state;
  // using generator_with_state<sir_state, any_sir_event >::future_state;
  int popsize = 10;
  epidemic_time_t tmax = 2;
  discrete_time_generator():
    generator_with_polled_state<sir_state,any_sir_event>(
						  default_state(),
						  update_if_satisfied,
						  should_update_current_state,
						  update_from_descendent
						  ) {
  };
  void generate(){
    running = false;
    current_value_mutex.lock();
    current_value = recovery_event(0,0);
    current_value_mutex.unlock();
    bool skip_next = true;
    event_counter = 0;
    running = true;
    ++event_counter;
    for (epidemic_time_t t = 0; t < tmax; ++t){
      std::vector<int> p1s;
      std::vector<int> p2s;
      p1s.reserve(current_state.population_size);
      p2s.reserve(current_state.population_size);
      // for (int p = 0 ; p < current_state.population_size; ++p){
      for (int p = current_state.population_size - 1; p > 1; --p){
	if(current_state.potential_states[p][S]){
	  p1s.push_back(p);
	}
	if(current_state.potential_states[p][I]){
	  p2s.push_back(p);
	}
      }

      for (auto p1 : p1s) {
	for (auto p2 : p2s) {
	  if(p1 == p2){
	    continue;
	  }
	  while(!is_ready()){
	    std::this_thread::yield();
	  }
	  current_value_mutex.lock();
	  current_value = infection_event(p1,p2,t);
	  generator_with_state<sir_state,any_sir_event>::update_state(future_state,current_value);
	  current_value_mutex.unlock();
	  ++event_counter;
	}
	if(skip_next){
	  skip_next = false;
	  continue;
	}
	while(!is_ready()){
	  std::this_thread::yield();
	}
	current_value_mutex.lock();
	current_value = recovery_event(p1,t);
	generator_with_state<sir_state,any_sir_event>::update_state(future_state,current_value);
	current_value_mutex.unlock();
	++event_counter;
      }
      auto tmp = current_value;
      std::visit([](auto&x){x.time += 1;},tmp);
      update_current_state(tmp);

      std::vector<int> fp1s;
      std::vector<int> fp2s;
      fp1s.reserve(future_state.population_size);
      fp2s.reserve(future_state.population_size);
      for (int p = 0 ; p < future_state.population_size; ++p){
	if(future_state.potential_states[p][I]){
	  fp1s.push_back(p);
	}
	if(future_state.potential_states[p][S]){
	  fp2s.push_back(p);
	}
      }
    }
    while(!is_ready()){
      std::this_thread::yield();
    }
    running = false;
  };
};

auto do_nothing = [](auto&x, auto&y){return;};

struct sir_filtered_generator : filtered_generator<sir_state, any_sir_event> {
public:
  sir_filtered_generator(
			 generator<any_sir_event>* _parent,
			 std::function<bool(const sir_state&, const any_sir_event&)> _filter
			 ) :
    filtered_generator<sir_state,any_sir_event>(
						default_state(),
						update_if_satisfied,
						should_update_current_state,
						_filter,
						do_nothing,
						_parent
						) {
  };
};
/*
 * Gillespie style emitting generator.  Uses a priority queue to emit events as their time threshold comes
 */

// typedef stxxl::VECTOR_GENERATOR<any_sir_event>::result any_sir_event_vector;
typedef std::vector<any_sir_event> any_sir_event_vector;

struct gillespie_generator : generator<any_sir_event> {
  using generator<any_sir_event >::running;
  int popsize = 10;
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
    for (person p1 = 0; p1 < popsize; ++p1) {
      for (person p2 = 0; p2 < popsize; ++p2) {
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
  int popsize = 10;
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
      for (person p1 = 0; p1 < popsize; ++p1) {
	for (person p2 = 0; p2 < popsize; ++p2) {
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



#endif

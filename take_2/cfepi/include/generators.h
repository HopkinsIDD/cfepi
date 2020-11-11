#ifndef __GENERATORS_H_
#define __GENERATORS_H_

#include <mutex>
#include <atomic>
#include <functional>
#include <iostream>

#define DEBUG_PRINT true
std::mutex printing_mutex;
template<typename T>
void print(const T&, std::string prefix);
void print(const std::string& x, std::string prefix = ""){
  std::cout << prefix << x << std::endl;
}

struct Exception {};

/*
 * @name generator
 * @description An abstract class for generating events.  There are three generators defined in this file
 *  - Emitting Generator : a class that emits events with no source.
 *  - Filtering Generator : a class that takes in events, processes them, and either releases them or doesn't
 *  - Resolving Generator : a class that takes in events, but does not release new ones.
 * Each generator is intended to be run in it's own thread, and to pass events between threads according to it's function.
 */

template<typename Event>
class generator {
public:
  std::atomic<bool> running = ATOMIC_VAR_INIT(false);
  Event current_value;
  std::vector<std::reference_wrapper<generator<Event> > > downstream_dependents;
  std::atomic<size_t> dependents_finished = ATOMIC_VAR_INIT(0);
  std::atomic<unsigned long long> event_counter = ATOMIC_VAR_INIT(0);
  std::mutex downstream_dependents_mutex;
  std::mutex downstream_finished_mutex;
  std::mutex current_value_mutex;
  void register_dependent(generator<Event>& dependent){
    downstream_finished_mutex.lock();
    downstream_dependents.push_back(dependent);
    downstream_finished_mutex.unlock();
  }
  void unregister_dependent(std::reference_wrapper<generator<Event> > dependent){
    downstream_finished_mutex.lock();
    downstream_dependents_mutex.lock();
    auto first_index_in_vector = std::find_if(
		 downstream_dependents.begin(),
		 downstream_dependents.end(),
		 [&dependent](std::reference_wrapper<generator<Event> > val){
		   return((&(val.get())) == (&(dependent.get())));
		 });
    if(std::end(downstream_dependents) != first_index_in_vector){
      downstream_dependents.erase(first_index_in_vector);
    } else {
      throw Exception();
    }
    downstream_dependents_mutex.unlock();
    downstream_finished_mutex.unlock();
  }
  void downstream_ready() {
    downstream_finished_mutex.lock();
    dependents_finished = dependents_finished.load() + 1;
    downstream_finished_mutex.unlock();
  }
  bool is_ready() {
    downstream_finished_mutex.lock();
    if(dependents_finished == downstream_dependents.size()){
      dependents_finished = 0;
      downstream_finished_mutex.unlock();
      return(true);
    }
    downstream_finished_mutex.unlock();
    return(false);
  }
  generator(){
    downstream_finished_mutex.lock();
    dependents_finished = 0;
    downstream_finished_mutex.unlock();
  }
  virtual void generate() = 0;
};


/*
 * @name generator_with_state
 * @description An abstract class for generating events based on a remembered state.  Provides functions for upkeeping said state as events come in
 */

template<typename State, typename Event>
class generator_with_state : public generator<Event> {
public:
  State current_state;
  State future_state;
  std::mutex current_state_mutex;
  const std::function<bool(const State&,const Event&)> should_update_state;
  const std::function<void(State&,const Event&)> update_state;
  void update_current_state(const Event& next_event){
    current_state_mutex.lock();
    if(should_update_state(current_state,next_event)){
      current_state = future_state;
    }
    current_state_mutex.unlock();
  }
  generator_with_state(
		       State initial_state,
		       std::function<void(State&,const Event&)> update_function,
		       std::function<bool(const State&,const Event&)> should_update_state
		       ):
    current_state(initial_state),
    future_state(initial_state),
    should_update_state(should_update_state),
    update_state(update_function){
  }
};

template<typename State, typename Event>
class generator_with_polled_state : public generator_with_state<State, Event> {
public:
  using generator_with_state<State,Event>::current_state;
  using generator_with_state<State,Event>::future_state;
  using generator_with_state<State,Event>::current_state_mutex;
  using generator_with_state<State,Event>::should_update_state;
  using generator_with_state<State,Event>::update_state;
  using generator<Event>::downstream_dependents;
  using generator<Event>::downstream_dependents_mutex;
  const std::function<void(State&, const State&)> update_state_from_dependent;
  void update_current_state(const Event& next_event){
    current_state_mutex.lock();
    if(DEBUG_PRINT){print(current_state, "before current: ");}
    if(DEBUG_PRINT){print(future_state, "before future:  ");}
    if(should_update_state(current_state,next_event)){
      current_state = future_state;
      downstream_dependents_mutex.lock();
      bool any_downstream = false;
      for(auto dependent : downstream_dependents){
	any_downstream = true;
	generator_with_state<State, Event>* dependent_pointer = dynamic_cast<generator_with_state<State,Event>* >(&dependent.get());
	if(dependent_pointer){
	  update_state_from_dependent(current_state,dependent_pointer->current_state);
	}
      }
      if(DEBUG_PRINT && any_downstream){std::string test = "updated from downstream"; print(test, "middle: ");}
      if(DEBUG_PRINT && (!any_downstream)){std::string test = "updated from future state"; print(test, "middle: ");}
      if(!any_downstream){
	current_state = future_state;
      }
      downstream_dependents_mutex.unlock();
    }
    if(DEBUG_PRINT){print(current_state, "after current: ");}
    if(DEBUG_PRINT){print(future_state, "after future:  ");}
    current_state_mutex.unlock();
  };
  generator_with_polled_state(
		       State initial_state,
		       std::function<void(State&,const Event&)> update_function,
		       std::function<bool(const State&,const Event&)> should_update_state,
		       std::function<void(State&, const State&)> update_state_from_dependent
			      ) : generator_with_state<State,Event>(initial_state,update_function,should_update_state),
				  update_state_from_dependent(update_state_from_dependent) {
  };
  // Plan is to create current state by polling the downstream states using the update_state_from_dependent function
};

/*
 * @name filtered_generator
 * @description This is the most important part of this code.  It takes in events from a source (parent), runs a filter to see if they pass through.  If they do pass through, then it passes them to any children it has.  This class hopefully won't need to be modified much if at all.
 */

template<typename State, typename Event>
class filtered_generator : public generator_with_polled_state<State, Event> {
public:
  std::function<bool(const State&, const Event&)> filter;
  unsigned long long parent_event_counter;
  generator<Event> *parent;
  using generator<Event>::running;
  using generator_with_state<State,Event>::current_state;
  using generator_with_state<State,Event>::future_state;
  void generate(){
    running = false;
    this->event_counter = 0;
    while(!parent->running){
      std::this_thread::yield();
    }
    parent_event_counter = 0;
    while((!running) & parent->running){
      while(parent->event_counter <= parent_event_counter){
	std::this_thread::yield();
      }
      ++parent_event_counter;
      auto value = parent -> current_value;
      generator_with_polled_state<State,Event>::update_current_state(value);
      if(filter(current_state, value)){
	this->current_value = value;
	generator_with_state<State,Event>::update_state(future_state, value);
	++this->event_counter;
	running = true;
      }
      parent->downstream_ready();
    }
    while(parent -> running){

      while((parent-> running ) & (parent->event_counter.load() <= parent_event_counter)){
	std::this_thread::yield();
      }
      if(parent -> running){
	parent_event_counter = parent->event_counter.load();
	auto value = parent -> current_value;
	if(filter(current_state, value)){
	  while(!this->is_ready()){
	    std::this_thread::yield();
	  }
	  this->current_value = value;
	  ++this->event_counter;
	}
      }
      parent->downstream_ready();
    }

    running = false;
  };
  filtered_generator(
		     State initial_state,
		     std::function<void(State&, const Event&)> state_update_function,
		     std::function<bool(const State&, const Event&)> should_state_update_function,
		     std::function<bool(const State&, const Event&)> _filter,
		     std::function<void(State&, const State&)> _poll,
		     generator<Event>* _parent
		     ) :
    generator_with_polled_state<State,Event>(initial_state, state_update_function, should_state_update_function, _poll),
    parent(_parent),
    filter(_filter)
  {
    parent -> register_dependent(*this);
  }
  ~filtered_generator(){
    parent -> unregister_dependent(*this);
  }
};

#endif

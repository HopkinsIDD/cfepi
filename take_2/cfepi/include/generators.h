#ifndef __GENERATORS_H_
#define __GENERATORS_H_

#include <mutex>
#include <atomic>
#include <functional>
#include <iostream>

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
  std::string name;
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
    downstream_dependents.erase(first_index_in_vector);
    downstream_dependents_mutex.unlock();
    downstream_finished_mutex.unlock();
  }
  void downstream_ready() {
    downstream_finished_mutex.lock();
    dependents_finished = dependents_finished.load() + 1;
    downstream_finished_mutex.unlock();
  }
  bool all_downstreams_ready() {
    return(dependents_finished == downstream_dependents.size());
  }
  bool is_ready() {
    downstream_finished_mutex.lock();
    if(all_downstreams_ready()){
      dependents_finished = 0;
      downstream_finished_mutex.unlock();
      return(true);
    }
    downstream_finished_mutex.unlock();
    return(false);
  }
  void generate(){
    initialize();
    Event event;
    while(more_events()){
      event = next_event();
      while(!is_ready()){
	std::this_thread::yield();
      }
      current_value_mutex.lock();
      current_value = event;
      current_value_mutex.unlock();
      ++event_counter;
    }
    running = false;
  }
  virtual Event next_event() = 0;
  virtual bool more_events() = 0;
  virtual void initialize() {
    event_counter = 0;
    running = true;
  };
  generator(std::string _name ) : name(_name){
    downstream_finished_mutex.lock();
    dependents_finished = 0;
    downstream_finished_mutex.unlock();
  }
};


/*
 * @name filtered_generator
 * @description This is the most important part of this code.  It takes in events from a source (parent), runs a filter to see if they pass through.  If they do pass through, then it passes them to any children it has.  This class hopefully won't need to be modified much if at all.
 */

template<typename Event>
class filtered_generator : virtual public generator<Event> {
public:
  unsigned long long parent_event_counter;
  generator<Event> *parent;
  using generator<Event>::name;
  virtual bool filter(const Event&) = 0;
  virtual void process(const Event&) = 0;
  virtual void initialize() {
    generator<Event>::initialize();
    this->event_counter = 0;
    parent_event_counter = 0;
    while(!parent->running){
      std::this_thread::yield();
    }
    parent->downstream_ready();
  }
  Event next_event(){
    auto value = parent -> current_value;
    process(value);
    parent->downstream_ready();
    return(value);
  };
  bool is_next_unfiltered_event(){

    while((parent->running) && (parent->event_counter <= parent_event_counter)){
      std::this_thread::yield();
    }
    if(parent->event_counter > parent_event_counter){
      return(true);
    }
    return(false);
  }
  bool more_events(){
    while(is_next_unfiltered_event()){
      if(filter(parent->current_value)){
	++parent_event_counter;
	return(true);
      } else {
	++parent_event_counter;
	parent->downstream_ready();
      }
    }
    return(false);
  };
  filtered_generator(generator<Event>* _parent, std::string _name = "filtered_generator : ") :generator<Event>(_name), parent(_parent) {
    parent -> register_dependent(*this);
  }
  ~filtered_generator(){
    parent -> unregister_dependent(*this);
  }
};
/*
 * @name generator_with_state
 * @description An abstract class for generating events based on a remembered state.  Provides functions for upkeeping said state as events come in
 */

template<typename State, typename Event>
class generator_with_state : virtual public generator<Event> {
public:
  State current_state;
  bool any_downstream_with_state;
  std::mutex current_state_mutex;
  using generator<Event>::downstream_dependents;
  using generator<Event>::name;
  using generator<Event>::downstream_dependents_mutex;
  virtual void update_state_from_all_downstream(const Event& next_event) = 0;
  void process(const Event& next_event){
    downstream_dependents_mutex.lock();
    if(any_downstream_with_state){
      update_state_from_all_downstream(next_event);
    } else {
      current_state_mutex.lock();
      update_state_from_event(next_event);
      current_state_mutex.unlock();
    }
    downstream_dependents_mutex.unlock();
  }
  generator_with_state(State initial_state, std::string _name = "generator_with_state : "): generator<Event>(_name), current_state(initial_state) {
  }
  virtual void initialize() {
    generator<Event>::initialize();
    downstream_dependents_mutex.lock();
    any_downstream_with_state = false;
    for(auto downstream : downstream_dependents){
      generator_with_state<State, Event>* downstream_pointer = dynamic_cast<generator_with_state<State,Event>* >(&downstream.get());
      if(downstream_pointer){
	any_downstream_with_state = true;
      }
    }
    downstream_dependents_mutex.unlock();
  }
  virtual void update_state_from_event(const Event&){
  };
};

template<typename State, typename Event>
class generator_with_buffered_state : virtual public generator_with_state<State, Event> {
public:
  using generator_with_state<State,Event>::current_state;
  using generator<Event>::name;
  State future_state;
  using generator_with_state<State,Event>::current_state_mutex;
  using generator<Event>::downstream_dependents;
  using generator<Event>::downstream_dependents_mutex;
  void update_state_from_event(const Event& next_event){
    apply_event_to_future_state(next_event);
    if(should_update_current_state(next_event)){
      update_state_from_buffer();
    }
  }
  void update_state_from_all_downstream(const Event& next_event){
    current_state_mutex.lock();
    while(!generator<Event>::all_downstreams_ready()){
      std::this_thread::yield();
    }

    if(should_update_current_state(next_event)){
      for(auto downstream : downstream_dependents){
	generator_with_state<State, Event>* downstream_pointer = dynamic_cast<generator_with_state<State,Event>* >(&downstream.get());
	if(downstream_pointer){
	  update_state_from_downstream(downstream_pointer);
	}
      }
    }
    current_state_mutex.unlock();
  }
  virtual bool should_update_current_state(const Event&) = 0;
  virtual void apply_event_to_future_state(const Event&) = 0;
  virtual void update_state_from_buffer() = 0;
  virtual void update_state_from_downstream(const generator_with_state<State,Event>*) = 0;
  generator_with_buffered_state(State initial_state, std::string _name = "generator_with_state : "): generator_with_state<State,Event>(initial_state, _name), future_state(initial_state) {
  };
  // Plan is to create current state by polling the downstream states using the update_state_from_dependent function
};


template<typename Event>
class basic_filtered_generator : public filtered_generator<Event>{
public:
  std::function<bool(const Event&)> user_filter;
  std::function<void(const Event&)> user_process;
  bool filter(const Event& event) {
    return(user_filter(event));
  };
  void process(const Event& event){
    user_process(event);
  };
  basic_filtered_generator(
			   generator<Event>* _parent,
			   std::function<bool(const Event&)> _filter,
			   std::function<void(const Event&)> _process,
			   std::string _name = "basic_filtered_generator : "
			   ) :
    generator<Event>(_name), filtered_generator<Event>(_parent, _name), user_filter(_filter), user_process(_process) {

  };
};
#endif

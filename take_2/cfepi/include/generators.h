#ifndef __GENERATORS_H_
#define __GENERATORS_H_

#include <mutex>
#include <atomic>
#include <functional>
#include <iostream>

#define DEBUG_PRINT false
#define DEBUG_EVENT_PRINT false
#define DEBUG_STREAM_PRINT false
#define DEBUG_STATE_PRINT false
#define DEBUG_EVENT_PERIOD 10000000
std::mutex printing_mutex;
template<typename T>
void print(const T&, std::string prefix);
void print(const std::string& x, std::string prefix = ""){
  std::cout << prefix << x << std::endl;
}
template<typename T>
void debug_print(const T& x, std::string prefix){
  if(DEBUG_PRINT){
    print(x,prefix);
  }
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
    if(std::end(downstream_dependents) != first_index_in_vector){
      downstream_dependents.erase(first_index_in_vector);
    } else {
      printing_mutex.lock();
      std::cout << name << "Could not un-register dependents" << std::endl;
      printing_mutex.unlock();
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
    if(DEBUG_PRINT){
      std::string msg = "generation begins";
      printing_mutex.lock();
      debug_print(msg,name);
      printing_mutex.unlock();
    }
    initialize();
    Event event;
    while(more_events()){
      event = next_event();
      if(DEBUG_EVENT_PRINT){
	printing_mutex.lock();
	std::string msg = "yielding";
	debug_print(msg,name);
	printing_mutex.unlock();
      }
      while(!is_ready()){
	std::this_thread::yield();
      }
      current_value_mutex.lock();
      current_value = event;
      current_value_mutex.unlock();
      ++event_counter;
      if(DEBUG_EVENT_PRINT || ((event_counter % DEBUG_EVENT_PERIOD) == 0)){
	printing_mutex.lock();
	std:: cout << name << "Generating event " << event_counter << std::endl;
	debug_print(event,name);
	printing_mutex.unlock();
      }
    }
    running = false;
  }
  virtual Event next_event() = 0;
  virtual bool more_events() = 0;
  virtual void initialize() {
    if(DEBUG_PRINT){
      std::string msg = "initialize as generator";
      printing_mutex.lock();
      debug_print(msg,name);
      printing_mutex.unlock();
    }
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
    if(DEBUG_PRINT){
      std::string msg = "initialize as filtered_generator";
      printing_mutex.lock();
      debug_print(msg,name);
      printing_mutex.unlock();
    }
    generator<Event>::initialize();
    this->event_counter = 0;
    parent_event_counter = 0;
    if(DEBUG_EVENT_PRINT){
      std::string msg = "waiting for parent";
      printing_mutex.lock();
      debug_print(msg,name);
      printing_mutex.unlock();
    }
    while(!parent->running){
      std::this_thread::yield();
    }
    if(DEBUG_EVENT_PRINT){
      std::string msg = "ready";
      printing_mutex.lock();
      debug_print(msg,name);
      printing_mutex.unlock();
    }
    parent->downstream_ready();
  }
  Event next_event(){
    if(DEBUG_EVENT_PRINT){
      printing_mutex.lock();
      std::cout << name << "Receiving event" << std::endl;
      printing_mutex.unlock();
    }
    auto value = parent -> current_value;
    process(value);
    parent->downstream_ready();
    return(value);
  };
  bool is_next_unfiltered_event(){
    if(DEBUG_EVENT_PRINT){
      printing_mutex.lock();
      std::cout << name << "yielding" << std::endl;
      std::cout << name << "waiting for parent event " << parent_event_counter + 1 << std::endl;
      std::cout << name << "currently on parent event " << parent->event_counter << std::endl;
      printing_mutex.unlock();
    }

    while((parent->running) && (parent->event_counter <= parent_event_counter)){
      if(DEBUG_EVENT_PRINT){
	printing_mutex.lock();
	// std::cout << name << "currently on parent event " << parent->event_counter << std::endl;
	printing_mutex.unlock();
      }
      std::this_thread::yield();
    }
    if(parent->event_counter > parent_event_counter){
      return(true);
    }
    return(false);
  }
  bool more_events(){
    while(is_next_unfiltered_event()){
      if(DEBUG_EVENT_PRINT){
	printing_mutex.lock();
	std::cout << name << "Looking for more events" << std::endl;
	printing_mutex.unlock();
      }
      if(filter(parent->current_value)){
	if(DEBUG_EVENT_PRINT){
	  printing_mutex.lock();
	  std::cout << name << "Accepted event" << std::endl;
	  print(parent->current_value,name);
	  printing_mutex.unlock();
	}
	++parent_event_counter;
	return(true);
      } else {
	if(DEBUG_EVENT_PRINT){
	  printing_mutex.lock();
	  std::cout << name << "Rejected event" << std::endl;
	  print(parent->current_value,name);
	  printing_mutex.unlock();
	}
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
      if(DEBUG_EVENT_PRINT){
	std::string msg = "Updating state from dependents";
	printing_mutex.lock();
	debug_print(msg,name);
	printing_mutex.unlock();
      }
      update_state_from_all_downstream(next_event);
    } else {
      if(DEBUG_EVENT_PRINT){
	std::string msg = "Updating state from event";
	printing_mutex.lock();
	debug_print(msg,name);
	printing_mutex.unlock();
      }
      current_state_mutex.lock();
      update_state_from_event(next_event);
      current_state_mutex.unlock();
    }
    if(DEBUG_STATE_PRINT){
      printing_mutex.lock();
      print(current_state,name + " state : ");
      printing_mutex.unlock();
    }
    downstream_dependents_mutex.unlock();
  }
  generator_with_state(State initial_state, std::string _name = "generator_with_state : "): generator<Event>(_name), current_state(initial_state) {
  }
  virtual void initialize() {
    if(DEBUG_PRINT){
      std::string msg = "initialize as generator_with_state";
      printing_mutex.lock();
      debug_print(msg,name);
      printing_mutex.unlock();
    }
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
    std::cout << "???" << std::endl;
    throw Exception();
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
    if(DEBUG_EVENT_PRINT && DEBUG_STATE_PRINT){
      std::string msg = "Applying event to future state";
      printing_mutex.lock();
      debug_print(msg,name);
      debug_print(next_event,name);
      print(future_state,name + " future_state: ");
      printing_mutex.unlock();
    }
    if(should_update_current_state(next_event)){
      if(DEBUG_PRINT){
	std::string msg = "Updating state from buffer";
	printing_mutex.lock();
	debug_print(msg,name);
	printing_mutex.unlock();
      }
      update_state_from_buffer();
    }
  }
  void update_state_from_all_downstream(const Event& next_event){
    if(DEBUG_STREAM_PRINT){
      printing_mutex.lock();
      std::cout << name << "Updating from all downstream" << std::endl;
      printing_mutex.unlock();
    }
    current_state_mutex.lock();
    while(!generator<Event>::all_downstreams_ready()){
      std::this_thread::yield();
    }

    if(should_update_current_state(next_event)){
      for(auto downstream : downstream_dependents){
	if(DEBUG_EVENT_PRINT){
	  printing_mutex.lock();
	  std::cout << name << downstream.get().name << "UPDATE" << std::endl;
	  printing_mutex.unlock();
	}
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

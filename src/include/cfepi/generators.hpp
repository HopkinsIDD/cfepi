#ifndef __GENERATORS_H_
#define __GENERATORS_H_

#include <mutex>
#include <atomic>
#include <functional>
#include <iostream>
#include <range/v3/all.hpp>

std::mutex printing_mutex;

template <typename generator_t>
concept generator_c = requires(const generator_t &cgr, generator_t &gr,
			       const generator_t cg, generator_t g) {
  typename generator_t::event_type;

  { initialize(gr) };

  { finalize(gr) };

  { more_events(gr) }
  ->std::convertible_to<bool>;

  { emit_event(gr) };

};

template<typename T>
void initialize(__attribute__((unused)) T&){}

template<typename T>
void finalize(__attribute__((unused)) T&){}

template<typename T>
requires requires(T tr){ {tr.current_value} -> std::same_as<typename T::event_type>; }
typename T::event_type current_value(const T& t){
  return(t.current_value);
}

template<typename generator_t>
requires generator_c<generator_t>
void generate(generator_t& gen){
  initialize(gen);
  while (more_events(gen)) {
    emit_event(gen);
  }
  finalize(gen);
}

template <typename object_t>
concept has_downstream_container_c = requires(object_t o, const object_t co,
                                              object_t &oref,
                                              const object_t &coref) {

  typename object_t::downstream_container_type;

  { co.downstream_container } -> ranges::forward_range;

  { *std::begin(co.downstream_container) } -> generator_c;

  { o.ready } -> std::same_as<std::atomic<bool> >;

  { lock_downstream_container(oref) };

  { unlock_downstream_container(oref) };

};

template <typename generator_t>
requires has_downstream_container_c<generator_t>
bool is_ready(generator_t & gen) {
  bool rc = gen.ready;
  lock_downstream_container(gen);
  for(auto dependent : gen.downstream_container){
    rc = rc && is_ready(dependent);
  }
  unlock_downstream_container(gen);
  return(rc);
}

/**********************************************************************************************
 * Mutex boilerplate                                                                          *
 *********************************************************************************************/

template <typename generator_t>
requires requires(generator_t t){ {t.current_value_mutex } -> std::same_as<std::mutex>; }
void lock_current_value(generator_t& x){
  x.current_value_mutex.lock();
}

template <typename generator_t>
requires requires(generator_t t){ {t.current_value_mutex } -> std::same_as<std::mutex>; }
void unlock_current_value(generator_t& x){
  x.current_value_mutex.unlock();
}

template <typename generator_t>
requires requires(generator_t t){ {t.downstream_container_mutex } -> std::same_as<std::mutex>; }
void lock_downstream_container(generator_t& x){
  x.downstream_container_mutex.lock();
}

template <typename generator_t>
requires requires(generator_t t){ {t.downstream_container_mutex } -> std::same_as<std::mutex>; }
void unlock_downstream_container(generator_t& x){
  x.downstream_container_mutex.unlock();
}

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
  }
  generator(std::string _name ) : name(_name){
    downstream_finished_mutex.lock();
    dependents_finished = 0;
    downstream_finished_mutex.unlock();
  }
  virtual ~generator() = default;
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
  virtual void initialize() override {
    generator<Event>::initialize();
    this->event_counter = 0;
    parent_event_counter = 0;
    while(!parent->running){
      std::this_thread::yield();
    }
    parent->downstream_ready();
  }
  Event next_event() override {
    auto value = parent -> current_value;
    process(value);
    parent->downstream_ready();
    return(value);
  }
  bool is_next_unfiltered_event(){

    while((parent->running) && (parent->event_counter <= parent_event_counter)){
      std::this_thread::yield();
    }
    if(parent->event_counter > parent_event_counter){
      return(true);
    }
    return(false);
  }
  bool more_events() override {
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
  }
  filtered_generator(generator<Event>* _parent) : parent(_parent) {
    parent -> register_dependent(*this);
  }
  virtual ~filtered_generator(){
    parent -> unregister_dependent(*this);
  }
};
/*
 * @name generator_with_state
 * @description An abstract class for generating events based on a remembered state.  Provides functions for upkeeping said state as events come in
 */

/*
template<typename State, typename Event>
class generator_with_state : virtual public generator<Event> {
public:
  State current_state;
  bool any_downstream_with_state;
  std::mutex current_state_mutex;
  using generator<Event>::downstream_dependents;
  // using generator<Event>::name;
  using generator<Event>::downstream_dependents_mutex;


  generator_with_state(State initial_state, std::string _name = "generator_with_state : "):
    generator<Event>(_name), current_state(initial_state) {
  }
};
*/

template<typename State, typename Event>
class generator_with_buffered_state : virtual public generator<Event> {
public:
  using event_type = Event;
  using generator<Event>::name;
  using generator<Event>::downstream_dependents;
  using generator<Event>::downstream_dependents_mutex;
  State current_state;
  State future_state;
  State initial_state;
  std::mutex current_state_mutex;
  bool any_downstream_with_state;
  void update_state_from_all_downstream(){
    printing_mutex.lock();
    print(current_state, name + "current : ");
    print(future_state, name + "future pre : ");
    printing_mutex.unlock();

    while(!generator<Event>::all_downstreams_ready()){
      std::this_thread::yield();
    }

    downstream_dependents_mutex.lock();
    if(any_downstream_with_state){
      for(auto downstream : downstream_dependents){
	generator_with_buffered_state<State, Event>* downstream_pointer = dynamic_cast<generator_with_buffered_state<State,Event>* >(&downstream.get());
	if(downstream_pointer){
	  downstream_pointer -> update_state_from_all_downstream();
	  update_state_from_downstream(downstream_pointer);
	}
      }
    }
    downstream_dependents_mutex.unlock();

    /*
    current_state_mutex.lock();
    current_state = future_state;
    current_state_mutex.unlock();

    printing_mutex.lock();
    print(future_state, name + "future post : ");
    printing_mutex.unlock();
    */

  }
  virtual void initialize() override {
    generator<Event>::initialize();
    current_state = initial_state;
    future_state = initial_state;
    downstream_dependents_mutex.lock();
    any_downstream_with_state = false;
    for(auto downstream : downstream_dependents){
      generator_with_buffered_state<State, Event>* downstream_pointer = dynamic_cast<generator_with_buffered_state<State,Event>* >(&downstream.get());
      if(downstream_pointer){
	any_downstream_with_state = true;
      }
    }
    downstream_dependents_mutex.unlock();
  }
  virtual void apply_event_to_future_state(const Event&) = 0;
  virtual void update_state_from_buffer() = 0;
  virtual void update_state_from_downstream(const generator_with_buffered_state<State,Event>*) = 0;
  generator_with_buffered_state(State _initial_state) :
    current_state(_initial_state), future_state(_initial_state), initial_state(_initial_state) {
  }
  // Plan is to create current state by polling the downstream states using the update_state_from_dependent function
};


template<typename Event>
class basic_filtered_generator : public filtered_generator<Event>{
public:
  std::function<bool(const Event&)> user_filter;
  std::function<void(const Event&)> user_process;
  bool filter(const Event& event) override {
    return(user_filter(event));
  }
  void process(const Event& event) override {
    user_process(event);
  }
  basic_filtered_generator(
			   generator<Event>* _parent,
			   std::function<bool(const Event&)> _filter,
			   std::function<void(const Event&)> _process,
			   std::string _name = "basic_filtered_generator : "
			   ) :
    generator<Event>(_name), filtered_generator<Event>(_parent), user_filter(_filter), user_process(_process) {

  }
};
#endif

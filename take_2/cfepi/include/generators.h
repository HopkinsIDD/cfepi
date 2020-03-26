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
  std::atomic<Event> current_value;
  std::atomic<int> downstream_dependents = ATOMIC_VAR_INIT(0);
  std::atomic<int> dependents_finished = ATOMIC_VAR_INIT(0);
  std::atomic<unsigned long long> event_counter = ATOMIC_VAR_INIT(0);
  std::mutex downstream_finished_mutex;
  std::mutex current_value_mutex;
  void register_dependent(){
    downstream_dependents = downstream_dependents.load() + 1;
  }
  void unregister_dependent(){
    downstream_dependents = downstream_dependents.load() - 1;
  }
  void downstream_ready() {
    downstream_finished_mutex.lock();
    dependents_finished = dependents_finished.load() + 1;
    downstream_finished_mutex.unlock();
  }
  bool is_ready() {
    downstream_finished_mutex.lock();
    if(dependents_finished == downstream_dependents){
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
    downstream_dependents = 0;
  }
  virtual void generate() = 0;
};


/*
 * @name filtered_generator
 * @description This is the most important part of this code.  It takes in events from a source (parent), runs a filter to see if they pass through.  If they do pass through, then it passes them to any children it has.  This class hopefully won't need to be modified much if at all.
 */
template<typename State, typename Event>
  class filtered_generator : public generator<Event> {
 public:
  std::function<bool(Event&,const State&, State&)> filter;
  unsigned long long parent_event_counter;
  generator<Event> *parent;
  State pre_event_state;
  State post_event_state;
  void generate(){
    this->running = false;
    this->event_counter = 0;
    while(!parent->running){
      std::this_thread::yield();
    }
    parent_event_counter = 0;
    while((!this->running) & parent->running){
      while(parent->event_counter <= parent_event_counter){
        std::this_thread::yield();
      }
      ++parent_event_counter;
      auto value = parent -> current_value.load();
      if(filter(value,this->pre_event_state,this->post_event_state)){
        this->current_value = value;
        ++this->event_counter;
        this->running = true;
	this->pre_event_state = this->post_event_state;
      }
      parent->downstream_ready();
    }
    while(parent -> running){

      while((parent-> running ) & (parent->event_counter.load() <= parent_event_counter)){
        std::this_thread::yield();
      }
      if(parent -> running){
	parent_event_counter = parent->event_counter.load();
	auto value = parent -> current_value.load();
	if(filter(value,pre_event_state,post_event_state)){
	  while(!this->is_ready()){
	    std::this_thread::yield();
	  }
	  this->current_value = value;
	  ++this->event_counter;
	  this->pre_event_state = this->post_event_state;
	}
      }
      parent->downstream_ready();
    }

    this->running = false;
  };
  filtered_generator(std::function<bool(Event&,const State&, State&)> _filter, generator<Event>* _parent) {
    parent = _parent;
    parent -> register_dependent();
    filter = _filter;
  }
  ~filtered_generator(){
    parent -> unregister_dependent();
  }
};

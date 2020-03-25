// this_thread::sleep_for example
#include <iostream>       // std::cout, std::endl
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds
// this_thread::yield example
#include <atomic>         // std::atomic
#include <functional>
#include <typeinfo>
#include <mutex>

using std::chrono::steady_clock;

/*
 * @name State
 * @description The state of the model.
 */
struct state {
  std::string prefix;
};

/*
 * @name Event
 * @description An event that transitions the model between states
 */
struct event {
  int value;
  event(int i){
    value = i;
  }
};

/*
 * @name generator
 * @description An abstract class for generating events.  There are three generators defined in this file
 *  - Emitting Generator : a class that emits events with no source.
 *  - Filtering Generator : a class that takes in events, processes them, and either releases them or doesn't
 *  - Resolving Generator : a class that takes in events, but does not release new ones.
 * Each generator is intended to be run in it's own thread, and to pass events between threads according to it's function.
 */
struct generator {
  std::atomic<bool> running = ATOMIC_VAR_INIT(false);
  std::atomic<event> current_value = ATOMIC_VAR_INIT(event(-1));
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
 * Sample emitting generator.  Emits numbers 1 through 1000
 */
struct counting_generator : generator {
  void generate(){
    running = false;
    current_value_mutex.lock();
    current_value = 0;
    current_value_mutex.unlock();
    event_counter = 0;
    running = true;
    ++event_counter;
    for (int i=1; i < 100000; ++i){
      while(!is_ready()){
        std::this_thread::yield();
        // std::this_thread::sleep_for(std::chrono::microseconds(1));
      }
      current_value_mutex.lock();
      current_value = event(i);
      current_value_mutex.unlock();
      ++event_counter;
    }
    while(!is_ready()){
      std::this_thread::yield();
    }
    running = false;
  };
};

/*
 * @name filtered_generator
 * @description This is the most important part of this code.  It takes in events from a source (parent), runs a filter to see if they pass through.  If they do pass through, then it passes them to any children it has.  This class hopefully won't need to be modified much if at all.
 */
template<typename State, typename Event>
struct filtered_generator : generator {
  std::function<bool(Event&,const State&, State&)> filter;
  unsigned long long parent_event_counter;
  generator *parent;
  State pre_event_state;
  State post_event_state;
  void generate(){
    running = false;
    event_counter = 0;
    while(!parent->running){
      std::this_thread::yield();
    }
    parent_event_counter = 0;
    while((!running) & parent->running){
      while(parent->event_counter <= parent_event_counter){
        std::this_thread::yield();
      }
      ++parent_event_counter;
      auto value = parent -> current_value.load();
      if(filter(value,pre_event_state,post_event_state)){
        current_value = value;
        ++event_counter;
        running = true;
	pre_event_state = post_event_state;
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
	  while(!is_ready()){
	    std::this_thread::yield();
	  }
	  current_value = value;
	  ++event_counter;
	  pre_event_state = post_event_state;
	}
      }
      parent->downstream_ready();
    }

    running = false;
  };
  filtered_generator(std::function<bool(Event&,const State&, State&)> _filter, generator* _parent) {
    parent = _parent;
    parent -> register_dependent();
    filter = _filter;
  }
  ~filtered_generator(){
    parent -> unregister_dependent();
  }
};

std::mutex printing_mutex;

std::function<bool(event&, const state&, state&)> printing_filter = [](event& x, const state& this_state, state& next_state){
  printing_mutex.lock();
  std::cout << this_state.prefix << x.value << std::endl;
  printing_mutex.unlock();
  return(true);
};

struct printing_generator : filtered_generator<state, event> {
  printing_generator(generator* _parent,std::string _prefix) : filtered_generator<state,event>(printing_filter, _parent){
    pre_event_state.prefix = _prefix;
    post_event_state.prefix = _prefix;
  };
};

std::function<bool(event&, const state&, state&)> filter_1 = [](event& x,const state& pre, state& post){
  return((x.value % 2) == 0);
};

std::function<bool(event&, const state&, state&)> filter_2 = [](event& x,const state& pre, state& post){
  return((x.value % 3) == 0);
};

int main ()
{
  counting_generator g;
  filtered_generator<state,event> f2_of_g(filter_2,&g);
  filtered_generator<state,event> f1_of_g(filter_1,&g);
  filtered_generator<state,event> f2_of_f1_of_g(filter_2,&f1_of_g);
  printing_generator out_1(&g,"unfiltered ");
  printing_generator out_2(&f1_of_g,"f1 ");
  printing_generator out_3(&f2_of_g,"f2 ");
  printing_generator out_4(&f2_of_f1_of_g,"both ");
  std::cout << "HERE" << std::endl;

  std::thread th8 = std::thread(&generator::generate,&out_1);
  std::thread th7 = std::thread(&generator::generate,&out_2);
  std::thread th6 = std::thread(&generator::generate,&out_3);
  std::thread th5 = std::thread(&generator::generate,&out_4);
  std::thread th4 = std::thread(&generator::generate,&f2_of_f1_of_g);
  std::thread th3 = std::thread(&generator::generate,&f2_of_g);
  std::thread th2 = std::thread(&generator::generate,&f1_of_g);
  std::thread th1 = std::thread(&generator::generate,&g);
  th1.join();
  th2.join();
  th3.join();
  th4.join();
  th5.join();
  th6.join();
  th7.join();
  th8.join();

  std::cout << "Final results" << std::endl;

  std::cout << g.event_counter << std::endl;
  std::cout << f1_of_g.event_counter << std::endl;
  std::cout << f2_of_g.event_counter << std::endl;
  std::cout << f2_of_f1_of_g.event_counter << std::endl;
  std::cout << std::endl;
  std::cout << out_1.event_counter << std::endl;
  std::cout << out_2.event_counter << std::endl;
  std::cout << out_3.event_counter << std::endl;
  std::cout << out_4.event_counter << std::endl;

  return 0;
}

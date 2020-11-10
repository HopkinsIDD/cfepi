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
#include <sir.h>

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
  int value = -1;
  event() noexcept {
    value = -1;
  }
  event(int i){
    value = i;
  }
};


/*
 * Sample emitting generator.  Emits numbers 1 through 1000
 */
struct counting_generator : generator<event> {
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


auto do_nothing = [](auto&x, auto&y){return;};

std::mutex printing_mutex;

auto printing_filter = [](const state& this_state, const event& x){
  printing_mutex.lock();
  std::cout << this_state.prefix << x.value << std::endl;
  printing_mutex.unlock();
  return(true);
};

struct printing_generator : filtered_generator<state, event> {
  printing_generator(generator<event>* _parent,std::string _prefix) :
    filtered_generator<state,event>(
				    state(),
				    [](state&, const event &){return;},
				    [](const state&, const event&){return(true);},
				    printing_filter,
				    do_nothing,
				    _parent
				    ){};
};

auto filter_1 = [](const state& pre, const event& x){
  return((x.value % 2) == 0);
};

auto filter_2 = [](const state& pre, const event& x){
  return((x.value % 3) == 0);
};

/*
 * stupid defaults
 */
auto always_update = [](const state&, const event&){return(true);};
auto empty_update = [](state&, const event&){return;};

int main ()
{
  counting_generator g;
  filtered_generator<state,event> f2_of_g(state(), empty_update, always_update, filter_2, do_nothing, &g);
  filtered_generator<state,event> f1_of_g(state(), empty_update, always_update, filter_1, do_nothing, &g);
  filtered_generator<state,event> f2_of_f1_of_g(state(), empty_update, always_update, filter_2, do_nothing, &f1_of_g);
  printing_generator out_1(&g,"unfiltered ");
  printing_generator out_2(&f1_of_g,"f1 ");
  printing_generator out_3(&f2_of_g,"f2 ");
  printing_generator out_4(&f2_of_f1_of_g,"both ");
  std::cout << "HERE" << std::endl;

  std::thread th8 = std::thread(&generator<event>::generate,&out_1);
  std::thread th7 = std::thread(&generator<event>::generate,&out_2);
  std::thread th6 = std::thread(&generator<event>::generate,&out_3);
  std::thread th5 = std::thread(&generator<event>::generate,&out_4);
  std::thread th4 = std::thread(&generator<event>::generate,&f2_of_f1_of_g);
  std::thread th3 = std::thread(&generator<event>::generate,&f2_of_g);
  std::thread th2 = std::thread(&generator<event>::generate,&f1_of_g);
  std::thread th1 = std::thread(&generator<event>::generate,&g);
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

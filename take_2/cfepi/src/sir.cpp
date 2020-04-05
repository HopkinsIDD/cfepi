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

std::mutex printing_mutex;

struct print_state {
  std::string prefix;
};

std::function<bool(any_sir_event&, const print_state&, print_state&)> printing_filter = [](any_sir_event& x, const print_state& this_state, print_state& next_state){
  printing_mutex.lock();
  /*
  std::cout << this_state.prefix;
  std::visit([](auto& event){
    std::cout << " : time " << event.time << " ";
    for(auto it : event.affected_people){
      std::cout << it << ", ";
    }
    std::cout << std::endl;
  },x);
  */
  printing_mutex.unlock();
  return(true);
};

struct printing_generator : filtered_generator<print_state, any_sir_event> {
  printing_generator(generator<any_sir_event>* _parent,std::string _prefix) : filtered_generator<print_state,any_sir_event>(printing_filter, _parent){
    current_state.prefix = _prefix;
    future_state.prefix = _prefix;
  };
};

int main () {
  gillespie_generator g;
  printing_generator out(&g,"initial");

  std::thread th1 = std::thread(&generator<any_sir_event>::generate,&g);
  std::thread th2 = std::thread(&generator<any_sir_event>::generate,&out);
  th1.join();
  th2.join();

  std::cout << "Final results" << std::endl;
  std::cout << g.event_counter << std::endl;
  std::cout << out.event_counter << std::endl;
}

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


struct print_state : public sir_state {
  std::string prefix;
  print_state(std::string prefix, sir_state initial_state = default_state()) : prefix(prefix), sir_state(initial_state) {};
};

auto printing_filter = [](const print_state& this_state, const any_sir_event& x){
  print(x);
  return(true);
};

void print(const print_state& x, std::string prefix = ""){
  const sir_state& y = x;
  print(y,prefix=prefix+x.prefix);
}

auto print_update_current_state = [](const print_state& state, const any_sir_event& event){
  print(state);
  return(should_update_current_state(state,event));
 };

struct printing_generator : filtered_generator<print_state, any_sir_event> {
  printing_generator(generator<any_sir_event>* _parent,std::string _prefix) :
    filtered_generator<print_state,any_sir_event>(
						  print_state(_prefix),
						  update_if_satisfied,
						  print_update_current_state,
						  printing_filter,
						  do_nothing,
						  _parent
						  ){};
};

std::function<bool(const sir_state&, const any_sir_event&)> tmpfun = [](const sir_state& state, const any_sir_event& event){
  bool rc = std::visit([](auto&x){
    for(auto p : x.affected_people){
      if(p == 3){
	// std::cout << "REJECTED" << std::endl;
	return(false);
      }
    }
    return(true);
  },
    event);
  return(rc);
 };



int main () {
  discrete_time_generator g;
  sir_filtered_generator f1_of_g(&g,tmpfun);
  // printing_generator out1(&g,"initial : ");
  // printing_generator out2(&f1_of_g,"filtered : ");

  std::thread th1 = std::thread(&generator<any_sir_event>::generate,&g);
  std::thread th2 = std::thread(&generator<any_sir_event>::generate,&f1_of_g);
  // std::thread th3 = std::thread(&generator<any_sir_event>::generate,&out1);
  // std::thread th4 = std::thread(&generator<any_sir_event>::generate,&out2);

  th1.join();
  th2.join();
  // th3.join();
  // th4.join();

  std::cout << "Final results" << std::endl;
  std::cout << g.event_counter << std::endl;
  std::cout << f1_of_g.event_counter << std::endl;
  // std::cout << out1.event_counter << std::endl;
  // std::cout << out2.event_counter << std::endl;

  print(g.current_state);
  print(f1_of_g.current_state);
  // print(out1.current_state);
  // print(out2.current_state);
}

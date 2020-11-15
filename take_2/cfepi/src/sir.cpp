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
  print_state(std::string prefix, sir_state initial_state = default_state()) : sir_state(initial_state), prefix(prefix) {};
};

void print(const print_state& x, std::string prefix = ""){
  const sir_state& y = x;
  print(y,prefix=prefix+x.prefix);
}

struct printing_generator : filtered_generator<any_sir_event> {
  using generator<any_sir_event>::name;
  bool filter(const any_sir_event& value){
    return true;
  }
  void process(const any_sir_event& value){
    print(value,name);
  }
  printing_generator(generator<any_sir_event>* _parent,std::string _name) : filtered_generator<any_sir_event>(_parent,_name){
  };
};

std::function<bool(const sir_state&, const any_sir_event&)> tmpfun = [](const sir_state& state, const any_sir_event& event){
  bool rc = std::visit([](auto&x){
    for(auto p : x.affected_people){
      if(p == 1){
	// std::cout << "REJECTED" << std::endl;
	return(false);
      }
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
  epidemic_time_t epidemic_length = 5;

  discrete_time_generator g(default_state(),epidemic_length,"Initial Generator : ");
  sir_filtered_generator f1_of_g(&g,default_state(), tmpfun, "Second Generator  : ");
  // printing_generator out1(&g,"initial : ");
  // printing_generator out2(&f1_of_g,"filtered : ");

  std::thread th1 = std::thread(&generator<any_sir_event>::generate,&g);
  std::thread th2 = std::thread(&sir_filtered_generator::generate,&f1_of_g);
  // std::thread th3 = std::thread(&generator<any_sir_event>::generate,&out1);
  // std::thread th4 = std::thread(&generator<any_sir_event>::generate,&out2);

  th1.join();
  th2.join();
  // th3.join();
  // th4.join();

  std::cout << "Final results" << std::endl;
  std::cout << g.event_counter << std::endl;
  // std::cout << f1_of_g.event_counter << std::endl;
  // std::cout << out1.event_counter << std::endl;
  // std::cout << out2.event_counter << std::endl;

  print(g.current_state);
  // print(f1_of_g.current_state);
  // print(out1.current_state);
  // print(out2.current_state);
}

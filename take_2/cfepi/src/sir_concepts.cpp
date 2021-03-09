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
#include <sir_generators.h>

#include <random>

using std::chrono::steady_clock;

std::random_device rd;
// std::default_random_engine random_source_1{rd()};
// std::default_random_engine random_source_2{rd()};
std::default_random_engine random_source_1{1};
std::default_random_engine random_source_2{1};
// std::default_random_engine random_source(rd());

struct print_state : public sir_state {
  std::string prefix;
  print_state(std::string prefix, sir_state initial_state = default_state()) : sir_state(initial_state), prefix(prefix) {}
};

void print(const print_state& x, std::string prefix = ""){
  const sir_state& y = x;
  print(y,prefix=prefix+x.prefix);
}

struct printing_generator : filtered_generator<any_sir_event> {
  using generator<any_sir_event>::name;
  bool filter(__attribute__((unused)) const any_sir_event& value) override {
    return true;
  }
  void process(const any_sir_event& value) override {
    print(value,name);
  }
  printing_generator(generator<any_sir_event>* _parent,std::string _name) :
    generator<any_sir_event>(_name),
    filtered_generator<any_sir_event>(_parent){
  }
};

std::function<bool(const sir_state&, const any_sir_event&)> filter_1 = [](__attribute__((unused)) const sir_state& state, __attribute__((unused)) const any_sir_event& event){
  return(true);
  /*
  auto size = std::visit([](auto&x){return(x.affected_people.size());},event);
  if(size <= 1){
    return(true);
  }

  std::uniform_real_distribution<double> random_probability(0.0,1.0);

  auto rval = random_probability(random_source_1);
  if(rval > .0003){
    return(false);
  }
  return(true);
  */
};

std::function<bool(const sir_state&, const any_sir_event&)> filter_2 = [](__attribute__((unused)) const sir_state& state, __attribute__((unused)) const any_sir_event& event){
  return(true);
  /*
std::function<bool(const sir_state&, const any_sir_event&)> filter_2 = [](__attribute__((unused)) const sir_state& state, const any_sir_event& event){
  auto size = std::visit([](auto&x){return(x.affected_people.size());},event);
  if(size <= 1){
    return(true);
  }

  std::uniform_real_distribution<double> random_probability(0.0,1.0);

  auto rval = random_probability(random_source_2);
  if(rval > .2){
    return(false);
  }
  bool rc = std::visit([](auto&x){
    for(auto p : x.affected_people) {
      if(p==3){return(false);}
    }
    return(true);
    },event);
  return(rc);
  */
};

int main () {
  epidemic_time_t epidemic_length = 4;
  auto initial_conditions = default_state(10);

  discrete_time_simple_generator g(initial_conditions,epidemic_length,"Initial Generator : ");
  // sir_filtered_generator f1_of_g(&g,initial_conditions, filter_1, "Second Generator  : ");
  // sir_filtered_generator f2_of_g(&g,initial_conditions, filter_2, "Third Generator  : ");
  // printing_generator out1(&g,"initial : ");
  // printing_generator out2(&f1_of_g,"filtered : ");

  generate(g);
  // std::thread th1 = std::thread(&generate<decltype(g)>, &g);
  // std::thread th2 = std::thread(&generator<any_sir_event>::generate,&f1_of_g);
  // std::thread th3 = std::thread(&sir_filtered_generator::generate,&f2_of_g);
  // std::thread th3 = std::thread(&generator<any_sir_event>::generate,&out1);
  // std::thread th4 = std::thread(&generator<any_sir_event>::generate,&out2);

  // th1.join();
  // th2.join();
  // th3.join();
  // th4.join();

  // std::cout << "Final results" << std::endl;
  // std::cout << g.event_counter << std::endl;
  // std::cout << f1_of_g.event_counter << std::endl;
  // std::cout << f2_of_g.event_counter << std::endl;
  // std::cout << out1.event_counter << std::endl;
  // std::cout << out2.event_counter << std::endl;

  // if(DEBUG_STATE_PRINT){
  print(g.current_state,g.name);
  // print(f1_of_g.current_state,"filter_1 current: ");
  // print(f2_of_g.current_state,"filter_2 current: ");
  // print(f1_of_g.future_state,"filter_1 future : ");
  // print(f2_of_g.future_state,"filter_2 future : ");
  // print(out1.current_state);
  // print(out2.current_state);
  //}
}

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
// #include <sir.h>

using std::chrono::steady_clock;

auto do_nothing = [](const auto&x){return;};

/*
 * @name State
 * @description The state of the model.
 */
struct state {
  std::string prefix;
};

void print(const state& s, std::string prefix = ""){
  std::cout << prefix << s.prefix << std::endl;
}
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

void print(const event& x, std::string prefix = ""){
  std::cout << prefix << x.value << std::endl;
}


/*
 * Sample emitting generator.  Emits numbers 1 through 1000
 */
struct counting_generator : generator<event> {
  int max_count;
  int current_count;
  bool more_events(){
    return(max_count > current_count);
  }
  event next_event(){
    event rc(current_count++);
    return(rc);
  }
  counting_generator(int max, int start = 0,std::string _name = "counting_generator : ") : generator<event>(_name), max_count(max), current_count(start) {
  };
};

struct printing_generator : filtered_generator<event> {
  using generator<event>::name;
  bool filter(const event& value){
    return true;
  }
  void process(const event& value){
    print(value,name);
    return;
  }
  printing_generator(generator<event>* _parent,std::string _name) : generator<event>(_name), filtered_generator<event>(_parent,_name){
  };
};

auto filter_1 = [](const event& x){
  return((x.value % 2) == 0);
};

auto filter_2 = [](const event& x){
  return((x.value % 3) == 0);
};


int main ()
{
  counting_generator g(100,0,"g            : ");
  basic_filtered_generator<event> f1_of_g(&g,filter_1,do_nothing,"f1_of_g       : ");
  basic_filtered_generator<event> f2_of_g(&g,filter_2,do_nothing,"f2_of_g       : ");
  basic_filtered_generator<event> f2_of_f1_of_g(&f1_of_g,filter_2, do_nothing, "f2_of_f1_of_g : ");
  printing_generator out_1(&g,"unfiltered ");
  printing_generator out_2(&f1_of_g,"f1 ");
  printing_generator out_3(&f2_of_g,"f2 ");
  printing_generator out_4(&f2_of_f1_of_g,"both ");
  std::cout << "HERE" << std::endl;

  std::thread th8 = std::thread(&generator<event>::generate,&out_4);
  std::thread th7 = std::thread(&generator<event>::generate,&out_3);
  std::thread th6 = std::thread(&generator<event>::generate,&out_2);
  std::thread th5 = std::thread(&generator<event>::generate,&out_1);
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

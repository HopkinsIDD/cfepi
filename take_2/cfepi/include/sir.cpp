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

typedef enum epidemic_state = {S, I, R, ncompartments};
typedef time_t float
typdef person int;

template<typename T>
/*
 * @name State
 * @description The state of the model.
 */
struct state {
  int population_size = 100;
  std::vector<bool[ncompartments]> potential_states;
  float beta = .2;
  float gamma = .1;
  std::string prefix;
};

/*
 * @name Event
 * @description An event that transitions the model between states
 */
/*
 * @name Event
 * @description An event that transitions the model between states
 */
template<int size>
struct sir_event {
  int my_size = size;
  epidemic_time_t time;
  std::array<person,size> affected_people;
  std::array<std::array<bool,ncompartments>,size> preconditions;
  std::array<std::pair<epidemic_state,bool>,size> postconditions;
  event();
  event(
        int _size,
        time_t _time,
        std::array<person,size> _affected_people,
        std::array<std::array<bool,ncompartments>,size> _preconditions,
        std::array<std::pair<epidemic_state,bool>,size> _postconditions
        ) : time(_time), affected_people(_affected_people), preconditions(_preconditions), postconditions(_postconditions) {}
};

struct infection_event : event<2> {
  std::array<std::array<bool,ncompartments>,2> preconditions = {std::array<bool,3>({true,false,false}),std::array<bool,3>({false,true,false})};
  std::array<std::pair<epidemic_state,bool>,2> postconditions = {std::make_pair(I,true), std::make_pair(I,false)};
  infection_event(person p1, person p2, epidemic_time_t _time) {
    this->time = _time;
    affected_people[0] = p1;
    affected_people[1] = p2;
  }
};

struct recovery_event : event<1> {
  std::array<std::array<bool,ncompartments>,1> preconditions = {std::array<bool,3>({false,true,false})};
  std::array<std::pair<epidemic_state,bool>,1> postconditions = {std::make_pair(R,true)};
  recovery_event(person p1, epidemic_time_t _time) {
    this->time = _time;
    affected_people[0] = p1;
  }
};

/*
 * Sample emitting generator.  Emits numbers 1 through 1000
 */
struct stupid_generator : generator<event> {
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

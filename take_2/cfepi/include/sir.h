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
#include <vector>
#include <variant>
#include <queue>

#ifndef __SIR_H_
#define __SIR_H_

enum epidemic_state {S, I, R, ncompartments};
typedef float epidemic_time_t;
typedef int person;

template<typename T>
/*
 * @name State
 * @description The state of the model.
 */
struct sir_state {
  int population_size = 0;
  std::vector<bool[ncompartments]> potential_states;
  float beta = .2;
  float gamma = .1;
  std::string prefix;
};


/*
 * @name Event
 * @description An event that transitions the model between states
 */
template<int size>
struct sir_event {
  epidemic_time_t time;
  std::array<person,size> affected_people;
  std::array<std::array<bool,ncompartments>,size> preconditions;
  std::array<std::variant<epidemic_state,bool>,size> postconditions;
};

struct infection_event : public sir_event<2> {
  using sir_event<2>::time;
  using sir_event<2>::affected_people;
  using sir_event<2>::preconditions;
  using sir_event<2>::postconditions;
  infection_event() noexcept = default;
  infection_event(const infection_event&) = default;
  infection_event(person p1, person p2, epidemic_time_t _time) {
    time = _time;
    affected_people = {p1, p2};
    preconditions = {std::array<bool,3>({true,false,false}),std::array<bool,3>({false,true,false})};
    postconditions = {I,false};
  }
};

struct recovery_event : public sir_event<1> {
  using sir_event<1>::time;
  using sir_event<1>::affected_people;
  using sir_event<1>::preconditions;
  using sir_event<1>::postconditions;
  recovery_event() : sir_event<1>() {};
  recovery_event(person p1, epidemic_time_t _time) {
    time = _time;
    affected_people[0] = p1;
    preconditions = {std::array<bool,3>({false,true,false})};
    postconditions = {R};
  }
};


auto time_compare = [](auto& x, auto&y){
  return(
	 std::visit(
		    [](auto &new_x, auto&new_y){
		      return(new_x.time > new_y.time);
		    },
		    x,
		    y
		    )
	 );
 };

typedef std::variant < recovery_event, infection_event > any_sir_event;
// using any_sir_event = sir_event<2>;
// using any_sir_event = sir_event;
/*
 * Sample emitting generator.  Emits numbers 1 through 1000
 */
struct gillespie_generator : generator<any_sir_event> {
  using generator<any_sir_event >::running;
  int popsize = 1000;
  epidemic_time_t tmax = 400;
  void generate(){
    running = false;
    epidemic_time_t current_time = 0;
    std::priority_queue<
      any_sir_event,
      std::vector<any_sir_event>,
      decltype(time_compare)
      > upcoming_events(time_compare);
    epidemic_time_t delta_time;
    for (person p1 = 0; p1 < popsize; ++p1) {
      for (person p2 = 0; p2 < popsize; ++p2) {
	delta_time = p1 + p2+1;
	upcoming_events.push(infection_event(p1,p2,current_time + delta_time));
      }
      delta_time = p1+1;
      upcoming_events.push(recovery_event(p1,current_time + delta_time));
    }
    current_value_mutex.lock();
    current_value = upcoming_events.top();
    delta_time = std::visit([](auto& x){return(x.affected_people[0]+1);},upcoming_events.top());
    auto tmp_val = upcoming_events.top();
    upcoming_events.pop();
    std::visit([delta_time](auto &x){x.time = x.time + delta_time;return;},tmp_val);
    upcoming_events.push(tmp_val);
    current_time = std::visit([](auto& x){return(x.time);},current_value);
    current_value_mutex.unlock();
    bool skip_next = true;
    event_counter = 0;
    running = true;
    ++event_counter;

    while (current_time < tmax) {
      std::cout << "time " << current_time << std::endl;
      while(!is_ready()){
	std::this_thread::yield();
	// std::this_thread::sleep_for(std::chrono::microseconds(1));
      }
      current_value_mutex.lock();
      current_value = upcoming_events.top();
      delta_time = std::visit([](auto& x){return(x.affected_people[0]+1);},upcoming_events.top());
      auto tmp_val = upcoming_events.top();
      upcoming_events.pop();
      std::visit([delta_time](auto &x){x.time = x.time + delta_time;return;},tmp_val);
      upcoming_events.push(tmp_val);
      current_time = std::visit([](auto& x){return(x.time);},current_value);
      current_value_mutex.unlock();
      ++event_counter;
    }
    while(!is_ready()){
      std::this_thread::yield();
    }
    running = false;
  };
};
struct stupid_generator : generator<any_sir_event> {
  using generator<any_sir_event >::running;
  int popsize = 1000;
  epidemic_time_t tmax = 10;
  void generate(){
    running = false;
    current_value_mutex.lock();
    current_value_mutex.unlock();
    current_value = recovery_event(0,0);
    bool skip_next = true;
    event_counter = 0;
    running = true;
    ++event_counter;
    for (epidemic_time_t t = 0; t < tmax; ++t){
      std::cout << "time " << t << std::endl;
      for (person p1 = 0; p1 < popsize; ++p1) {
	for (person p2 = 0; p2 < popsize; ++p2) {
	  while(!is_ready()){
	    std::this_thread::yield();
	    // std::this_thread::sleep_for(std::chrono::microseconds(1));
	  }
	  current_value_mutex.lock();
	  current_value = infection_event(p1,p2,t);
	  current_value_mutex.unlock();
	  ++event_counter;
	}
	if(skip_next){
	  skip_next = false;
	  std::cout << "EHRE" << std::endl;
	  continue;
	}
	while(!is_ready()){
	  std::this_thread::yield();
	  // std::this_thread::sleep_for(std::chrono::microseconds(1));
	}
	current_value_mutex.lock();
	current_value = recovery_event(p1,t);
	current_value_mutex.unlock();
	++event_counter;
      }
    }
    while(!is_ready()){
      std::this_thread::yield();
    }
    running = false;
  };
};

#endif

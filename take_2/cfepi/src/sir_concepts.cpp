// this_thread::yield example
// #include <sir_generators.h>

#include <chrono>         // std::chrono::seconds
#include <random>

#include <sir.h>
#include <ThreadPool/ThreadPool.h>

using std::chrono::steady_clock;

std::random_device rd;
std::default_random_engine random_source_1{rd()};
std::default_random_engine random_source_2{rd()};
// std::default_random_engine random_source_1{1};
// std::default_random_engine random_source_2{1};
std::uniform_real_distribution<double> random_probability(0.0,1.0);


template <auto Start, auto End, auto Inc, class F>
constexpr void constexpr_for(F&& f) {
    if constexpr (Start < End)
    {
        f(std::integral_constant<decltype(Start), Start>());
        constexpr_for<Start + Inc, End, Inc>(f);
    }
}

void update_current_state_from_future_state(sir_state& current_state, sir_state& future_state){
  auto population_size = current_state.potential_states.size();
  // This doesn't really work in the case of actual potential states (just real states)
  for(auto person_index : ranges::views::iota( 0UL ) | ranges::views::take(population_size)){
    bool any_set = false;
    for(auto this_set : future_state.potential_states[person_index]){
      any_set = any_set || this_set;
    }
    if(!any_set){
      future_state.potential_states[person_index] = current_state.potential_states[person_index];
    }
  }
  current_state = future_state;
}

template<size_t event_type_index>
struct probability_of_event_type {};

template<>
struct probability_of_event_type<0> {
  constexpr const static double value = 1.0;
};

template<>
struct probability_of_event_type<1> {
  constexpr const static double value =  0.1;
};

template<>
struct probability_of_event_type<2> {
  constexpr const static double value = .3 / 3000;
};

sir_state run_simulation(sir_state& initial_state, epidemic_time_t max_epidemic_time, size_t n_cores = std::thread::hardware_concurrency()){
  auto current_state = initial_state;
  auto future_state = current_state;
  std::cout << "Using " << n_cores << " cores\n";

  ThreadPool pool(n_cores);


  std::atomic<size_t> events_in_progress = 0;
  std::atomic<size_t> total_events = 0;

  epidemic_time_t current_epidemic_time = 0;
  while(current_epidemic_time <= max_epidemic_time){
    // std::cout << "Looping" << "\n";

    current_epidemic_time += 1;
    // auto event_generator = single_time_event_generator<number_of_event_types>(current_state);
    // auto event_range = event_generator.event_range();

    constexpr_for<0,number_of_event_types,1>([&events_in_progress, &total_events, &current_state, &future_state, &pool, &current_epidemic_time](auto event_index){
					       // std::cout << "Generating events of type " << event_index << "\n";
					       auto event_range_generator = cartesian_multiproduct<event_index>(current_state);
					       auto event_range = event_range_generator.event_range();
					       auto event_iterator = ranges::begin(event_range);
					       // std::cout << "  event iterator created\n";
					       //  constexpr std::geometric_distribution<float> geometric_r(std::integral_constant<probability_of_event_type<event_index>::value >);
					       std::geometric_distribution geometric_r(probability_of_event_type<event_index>::value);

					       auto event_increment = geometric_r(random_source_1);
					       // event_increment = 0;
					       // std::cout << "  event iterator initially incremented by" << event_increment << "\n";
					       event_iterator = ranges::next(event_iterator, event_increment, ranges::end(event_range));
					       // std::cout << "  event iterator initially increment success \n";

					       while(event_iterator != ranges::end(event_range)){
					         // std::cout << "    inner event iterator loop iteration\n";

						 auto event = *event_iterator;
						 std::visit(any_sir_event_set_time{current_epidemic_time},event);

						 pool.enqueue(
							      [&events_in_progress,&total_events,&future_state](const any_sir_event& this_event){
								std::visit([&future_state](const auto& x){
									     /*
									     auto rval = random_probability(random_source_1);
									     if((x.affected_people.size() == 1) && (rval > .3)){
									       return;
									     }
									     if((x.affected_people.size() > 1) && (rval > .003)){
									       return;
									     }
									     */
									     any_sir_event_apply_to_sir_state{future_state}(x);
									   }, this_event);
								--events_in_progress;
								++total_events;
							      },
							      event
							      );
						 // process_event, event, events_in_progress, current_state, future_state
						 ++events_in_progress;

						 event_increment = geometric_r(random_source_1) + 1;
						 // event_increment = 1;


						 // std::cout << "    event iterator incremented by" << event_increment << "\n";
						 event_iterator = ranges::next(event_iterator, event_increment, ranges::end(event_range));
						 // std::cout << "    event iterator increment success \n";
					       }
					     });
    while(events_in_progress > 0){
      // std::cout << "  events in progress " << events_in_progress << "\n";
    }
    // std::cout << "Current time is " << current_epidemic_time << "/" << max_epidemic_time << "\n";
    // std::cout << "  total events so far : " << total_events << "\n";
    update_current_state_from_future_state(current_state, future_state);
  }
  return(current_state);
}

int main () {

  epidemic_time_t epidemic_length = 365;
  auto initial_conditions = default_state(3000);

  auto start = std::chrono::steady_clock::now();
  for(auto& elem : initial_conditions.potential_states){
    for(auto& elem2 : elem){
      // elem2 = true;
    }
  }
  std::cout << (std::chrono::steady_clock::now() - start).count() << " seconds startup\n";

  start = std::chrono::steady_clock::now();
  auto final_state = run_simulation(initial_conditions, epidemic_length, 1);
  std::cout << (std::chrono::steady_clock::now() - start).count() << " seconds running single threaded\n";
  start = std::chrono::steady_clock::now();
  final_state = run_simulation(initial_conditions, epidemic_length);
  std::cout << (std::chrono::steady_clock::now() - start).count() << " seconds running\n";
  print(final_state);

}

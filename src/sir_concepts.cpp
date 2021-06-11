// this_thread::yield example
// #include <sir_generators.h>

#include <chrono>         // std::chrono::seconds
#include <random>
#include <functional>

#include <cfepi/sir.hpp>
#include <ThreadPool/ThreadPool.h>

using std::chrono::steady_clock;
using namespace std::chrono_literals;

std::random_device rd;
std::default_random_engine random_source_1{1};
std::default_random_engine random_source_2{1};
std::uniform_real_distribution<double> random_probability(0.0,1.0);

constexpr person_t population_size = 3;

template <auto Start, auto End, auto Inc, class F>
constexpr void constexpr_for(F&& f) {
    if constexpr (Start < End)
    {
        f(std::integral_constant<decltype(Start), Start>());
        constexpr_for<Start + Inc, End, Inc>(f);
    }
}

void reset_temporary_state(sir_state& state){
  for(auto person_index : ranges::views::iota( 0UL ) | ranges::views::take(state.potential_states.size())){
    for(auto state_index : ranges::views::iota( 0UL ) | ranges::views::take(ncompartments)){
      state.potential_states[person_index][state_index] = false;
    }
  }
}

void update_current_state_from_future_state(sir_state& current_state, sir_state& entered_states, sir_state& left_states){
  auto current_population_size = current_state.potential_states.size();
  // This doesn't really work in the case of actual potential states (just real states)
  for(auto person_index : ranges::views::iota( 0UL ) | ranges::views::take(current_population_size)){
    for(auto state_index : ranges::views::iota( 0UL ) | ranges::views::take(ncompartments)){
      current_state.potential_states[person_index][state_index] = (
	current_state.potential_states[person_index][state_index] &&
	(!left_states.potential_states[person_index][state_index])
      ) ||
	entered_states.potential_states[person_index][state_index];
    }
  }
  reset_temporary_state(entered_states);
  reset_temporary_state(left_states);
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
  constexpr const static double value = .3 / population_size;
};

auto process_single_event_no_context =
    [](const any_sir_event &this_event, sir_state &local_entered_states,
       sir_state &local_left_states,
       const std::vector<
           std::vector<std::function<bool(const any_sir_event &)>>> &filters,
       std::atomic<size_t> &events_in_progress,
       std::atomic<size_t> &total_events
    ) {
      bool any_filter_accepted = false;
      for (auto filter_set : filters) {
        bool this_filter_accepted = true;
        for (auto filter : filter_set) {
          this_filter_accepted = this_filter_accepted && filter(this_event);
        }
        any_filter_accepted = any_filter_accepted || this_filter_accepted;
      }
      if (any_filter_accepted) {

        std::visit(any_sir_event_apply_entered_states{local_entered_states},
                   this_event);

        std::visit(any_sir_event_apply_left_states{local_left_states},
                   this_event);
        --events_in_progress;
        ++total_events;
      }
    };

auto run_single_timestep_no_context = [](const sir_state &current_state,
                                         sir_state &local_entered_states,
                                         sir_state &local_left_states,
                                         epidemic_time_t current_epidemic_time,
					 const std::vector<std::vector<std::function<bool(const any_sir_event&)> > >& filters,
                                         std::atomic<size_t>
                                             &events_in_progress,
                                         std::atomic<size_t> &total_events,
                                         ThreadPool &pool, auto event_index,
					 std::default_random_engine random_source
) {

  // Create event iterators
  auto event_range_generator =
    cartesian_multiproduct<event_index>(current_state);
  auto event_range = event_range_generator.event_range();
  auto event_iterator = ranges::begin(event_range);

  // Create next stepsize distribution
  std::geometric_distribution geometric_r(
    probability_of_event_type<event_index>::value);
  auto event_increment = geometric_r(random_source);

  event_iterator =
    ranges::next(event_iterator, event_increment, ranges::end(event_range));

  while (event_iterator != ranges::end(event_range)) {
    // std::cout << "    inner event iterator loop iteration\n";
    while (events_in_progress > 10) {
      std::this_thread::yield();
    }

    auto event = *event_iterator;
    std::visit(any_sir_event_set_time{current_epidemic_time}, event);

    auto process_single_event = std::bind(
      process_single_event_no_context,
      std::placeholders::_1,
      std::ref(local_entered_states),
      std::ref(local_left_states),
      std::cref(filters),
      std::ref(events_in_progress),
      std::ref(total_events)
    );

    pool.enqueue(
      process_single_event,
      event);
    // process_event, event, events_in_progress, current_state,
    // future_state
    ++events_in_progress;

    event_increment = geometric_r(random_source) + 1;
    event_iterator =
      ranges::next(event_iterator, event_increment, ranges::end(event_range));
  }
};

sir_state run_simulation(
  sir_state initial_state,
  epidemic_time_t max_epidemic_time,
  const std::vector<std::vector<std::function<bool(const any_sir_event&)> > > &filters,
  size_t n_cores = std::thread::hardware_concurrency()-1,
  std::default_random_engine random_source = std::default_random_engine{rd()}
){

  auto current_state = initial_state;

  auto entered_states = current_state;
  auto left_states = current_state;
  reset_temporary_state(entered_states);
  reset_temporary_state(left_states);

  std::cout << "Using " << n_cores << " cores\n";

  ThreadPool pool(n_cores);

  std::atomic<size_t> events_in_progress = 0;
  std::atomic<size_t> total_events = 0;

  epidemic_time_t current_epidemic_time = 0;


  auto run_single_timestep = std::bind(
    run_single_timestep_no_context,
    std::placeholders::_1,
    std::placeholders::_2,
    std::placeholders::_3,
    std::placeholders::_4,
    std::cref(filters),
    std::ref(events_in_progress),
    std::ref(total_events),
    std::ref(pool),
    std::placeholders::_5,
    std::ref(random_source)
  );

  while(current_epidemic_time <= max_epidemic_time){

    current_epidemic_time += 1;

    auto run_this_timestep = std::bind(
      run_single_timestep,
      std::cref(current_state),
      std::ref(entered_states),
      std::ref(left_states),
      current_epidemic_time,
      std::placeholders::_1
    );

    constexpr_for<0, number_of_event_types, 1>(run_this_timestep);

    while(events_in_progress > 0){
      std::this_thread::yield();
    }

    update_current_state_from_future_state(current_state, entered_states, left_states);
  }
  return(current_state);
}

int main () {

  epidemic_time_t epidemic_length = 365;
  const auto initial_conditions = default_state(population_size);

  std::vector<std::vector<std::function<bool(const any_sir_event &)>>> filters =
      {{[](const any_sir_event &) { return (false); }},
       {[](const any_sir_event &) { return (true); }}};

  auto start = std::chrono::steady_clock::now();
  for(auto& elem : initial_conditions.potential_states){
    for (auto __attribute__((unused)) &elem2 : elem) {
      // elem2 = true;
    }
  }
  std::cout << (std::chrono::steady_clock::now() - start).count() << " seconds startup\n";

  start = std::chrono::steady_clock::now();
  auto final_state = run_simulation(initial_conditions, epidemic_length, filters, 1, random_source_1);
  std::cout << (std::chrono::steady_clock::now() - start).count() << " seconds running single threaded\n";
  print(final_state);

  start = std::chrono::steady_clock::now();
  final_state =
      run_simulation(initial_conditions, epidemic_length, filters,
                     std::thread::hardware_concurrency() - 1, random_source_2);
  std::cout << (std::chrono::steady_clock::now() - start).count() << " seconds running\n";
  print(final_state);

}

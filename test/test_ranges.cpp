#include <array>
#include <vector>
#include <iostream>
#include <range/v3/all.hpp>

#include <chrono>

// #include <cppcoro/generator.hpp>
#include "/home/jkaminsky/git/cfepi/take_2/cfepi/include/sir.h"

constexpr void empty_function(auto... x) {};

template<typename T,size_t i>
using repeat = T;

void update_from_descendent(sir_state &ours, const sir_state &theirs) {
  if (theirs.time > ours.time) {
    ours.time = theirs.time;
    for (person_t person = 0; person < ours.population_size; ++person) {
      for (size_t compartment = 0; compartment < ncompartments; ++compartment) {
        ours.potential_states[person][compartment] = false;
      }
    }
  }
  for (person_t person = 0; person < ours.population_size; ++person) {
    for (size_t compartment = 0; compartment < ncompartments; ++compartment) {
      ours.potential_states[person][compartment] =
          ours.potential_states[person][compartment] ||
          theirs.potential_states[person][compartment];
    }
  }
};

template<size_t event_index, size_t precondition_index>
bool check_event_precondition(std::array<bool, number_of_event_types> x){
  bool rc = false;
  auto tmp = sir_event_true_preconditions<event_index, precondition_index>::value;
  for(auto elem : tmp){
    rc = rc || x[elem];
  }
  return(rc);
}

template<
  size_t event_index,
  size_t precondition_index
  >
struct filter_population_by_event_preconditions{
  static auto get_filtered_reference_vectors(const auto & single_gen){
    auto vector_start = ranges::begin(single_gen);

    return(ranges::to<std::vector<size_t> >(
	ranges::transform_view(
	  ranges::filter_view(single_gen, &check_event_precondition<event_index,precondition_index>),
	  [&vector_start](auto & x){ return(&x - vector_start); }
	)
      ));
  }
};

template<size_t event_index>
const auto transform_lambda = [](auto&& x){
  sir_event_by_index<event_index> rc;
  std::apply([&rc](auto... y){rc.affected_people = {y...};}, x);
  return((any_sir_event) rc);
 };

template<size_t event_index, typename = std::make_index_sequence<event_size_by_event_index<event_index>::value > >
struct cartesian_multiproduct;

template<size_t event_index, size_t... precondition_index>
struct cartesian_multiproduct<event_index,std::index_sequence<precondition_index...> >{
  std::tuple<repeat<std::vector<size_t>, event_size_by_event_index<precondition_index>::value>... > vectors;
  constexpr const static auto apply_lambda = [](auto&... x){return(ranges::views::cartesian_product(x...));};
  auto cartesian_range(){
    return(std::apply(apply_lambda, vectors));
  };
  auto event_range(){
    return(ranges::transform_view(cartesian_range(), transform_lambda<event_index>));
  };
  // cartesian_multiproduct(std::tuple<repeat<std::vector<size_t>, event_size_by_event_index<precondition_index>::value>... > _vectors) : vectors(std::move(_vectors)) {};
  // cartesian_multiproduct(const cartesian_multiproduct& _from) : vectors(std::move(_from.vectors)) {};
  cartesian_multiproduct() = default;
  cartesian_multiproduct(const sir_state& current_state) {
    auto count_gen = ranges::span(current_state.potential_states);
    vectors = std::make_tuple(filter_population_by_event_preconditions<event_index,precondition_index>::get_filtered_reference_vectors(count_gen)...);
  }
};

template<typename T, size_t N = number_of_event_types, typename = std::make_index_sequence<N> >
struct any_event_type_range;

template<typename T, size_t N, size_t... event_index>
struct any_event_type_range<T, N, std::index_sequence<event_index...> > :
  std::variant<decltype(cartesian_multiproduct<event_index>().event_range())...> {};

template <size_t N = number_of_event_types,
          typename = std::make_index_sequence<N>>
struct single_time_event_generator;

template <size_t N, size_t... event_index>
struct single_time_event_generator<N, std::index_sequence<event_index...>> {

  std::tuple<cartesian_multiproduct<event_index>...> product_thing;

  constexpr const static auto concat_lambda = [](auto &...x) {
    return (ranges::views::concat(x.event_range()...));
  };

  std::variant<decltype(std::get<event_index>(product_thing).event_range())...>
      range_variant;
  /*
    cppcoro::generator<any_sir_event> test(){
    for(any_sir_event elem : std::get<0>(product_thing).event_range()){
    co_yield elem;
    }
    }
  */

  auto event_range() { return (std::apply(concat_lambda, product_thing)); };

  single_time_event_generator() = default;
  single_time_event_generator(sir_state &initial_conditions)
      : product_thing(
            cartesian_multiproduct<event_index>(initial_conditions)...){};
};

template <size_t event_index>
size_t iterate_through(sir_state &initial_conditions) {
  size_t counter = 0;
  cartesian_multiproduct<event_index> gen(initial_conditions);
  auto event_range = gen.event_range();

  for (auto elem : event_range) {
    ++counter;
  }
  return (counter);
};

size_t iterate_through(sir_state &initial_conditions) {
  size_t counter = 0;
  single_time_event_generator<number_of_event_types> gen(initial_conditions);
  auto event_range = gen.event_range();
  for (auto elem : event_range) {
    ++counter;
  }
  return (counter);
};

int main(int argc, char **argv) {

  if (argc != 3) {
    std::cout
        << "This program takes 2 runtimes arguments:\n\t- The number of people "
           "in the population\n\t- The number of time steps to run over\n";
    return -1;
  }
  size_t state_size = atoi(argv[1]);
  epidemic_time_t epidemic_length = atoi(argv[2]);
  std::cout << "Running " << epidemic_length
            << " time steps on a population of size " << state_size << "\n";

  std::string prefix = "main : ";
  sir_state initial_conditions = default_state(state_size);
  decltype(std::chrono::steady_clock::now() -
           std::chrono::steady_clock::now()) b1_prev_elapsed;
  decltype(std::chrono::steady_clock::now() -
           std::chrono::steady_clock::now()) b2_prev_elapsed;
  for (size_t elem : ranges::views::iota(0, epidemic_length)) {
    if (elem < state_size) {
      initial_conditions.potential_states[elem][I] = true;
    }

    auto b2_start = std::chrono::steady_clock::now();
    iterate_through(initial_conditions);
    auto b2_end = std::chrono::steady_clock::now();
    auto b2_elapsed = b2_end - b2_start;

    auto b1_start = std::chrono::steady_clock::now();
    iterate_through<0>(initial_conditions);
    iterate_through<1>(initial_conditions);
    iterate_through<2>(initial_conditions);
    auto b1_end = std::chrono::steady_clock::now();
    auto b1_elapsed = b1_end - b1_start;

    std::cout << "Unified : " << b2_elapsed.count() << "\n"
              << "Separate : " << b1_elapsed.count() << "\n"
              << "Ratio : " << b1_elapsed.count() * 1. / b2_elapsed.count()
              << "\n"
              << "Unified Diff : "
              << b2_elapsed.count() - b2_prev_elapsed.count() << "\n"
              << "Separate Diff : "
              << b1_elapsed.count() - b1_prev_elapsed.count() << "\n"
              << "Diff Ratio : "
              << (b1_elapsed.count() - b1_prev_elapsed.count()) * 1. /
                     (b2_elapsed.count() - b2_prev_elapsed.count())
              << "\n";
    b1_prev_elapsed = b1_elapsed;
    b2_prev_elapsed = b2_elapsed;
  }
}

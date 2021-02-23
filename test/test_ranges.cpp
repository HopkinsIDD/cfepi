#include <array>
#include <vector>
#include <iostream>
#include <range/v3/all.hpp>
// #include <cppcoro/generator.hpp>
#include "/home/jkaminsky/git/cfepi/take_2/cfepi/include/sir.h"

constexpr void empty_function(auto... x) {};

template<typename T,size_t i>
using repeat = T;

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
  static auto get_filtered_reference_vectors(auto && single_gen){
    auto vector_start = ranges::begin(single_gen);

    return(ranges::to<std::vector<size_t> >(
	ranges::transform_view(
	  ranges::filter_view(single_gen, &check_event_precondition<event_index,precondition_index>),
	  [&vector_start](auto & x){ return(&x - vector_start); }
	)
      ));
  }
};

template<size_t event_index, typename = std::make_index_sequence<event_size_by_event_index<event_index>::value > >
struct cartesian_multiproduct;

template<size_t event_index, size_t... precondition_index>
struct cartesian_multiproduct<event_index,std::index_sequence<precondition_index...> >{
  std::tuple<repeat<std::vector<size_t>, event_size_by_event_index<precondition_index>::value>... > vectors;
  constexpr const static auto apply_lambda = [](auto&... x){return(ranges::views::cartesian_product(x...));};
  constexpr const static auto transform_lambda = [](auto&& x){
    sir_event_by_index<event_index> rc;
    std::apply([&rc](auto... y){rc.affected_people = {y...};}, x);
    return((any_sir_event) rc);
  };
  auto cartesian_range(){
    return(std::apply(apply_lambda, vectors));
  };
  auto event_range(){
    return(ranges::transform_view(cartesian_range(), transform_lambda));
  };
  // cartesian_multiproduct(std::tuple<repeat<std::vector<size_t>, event_size_by_event_index<precondition_index>::value>... > _vectors) : vectors(std::move(_vectors)) {};
  // cartesian_multiproduct(const cartesian_multiproduct& _from) : vectors(std::move(_from.vectors)) {};
  cartesian_multiproduct() = default;
  cartesian_multiproduct(const sir_state& current_state) {
    auto count_gen = ranges::span(current_state.potential_states);
    vectors = std::make_tuple(filter_population_by_event_preconditions<event_index,precondition_index>::get_filtered_reference_vectors(count_gen)...);
  }
};

template<size_t N = number_of_event_types, typename = std::make_index_sequence<N> >
struct single_time_event_generator;

template<size_t N, size_t... event_index>
struct single_time_event_generator<N, std::index_sequence<event_index...> > {

  std::tuple<cartesian_multiproduct<event_index>...> product_thing;

  constexpr const static auto concat_lambda = [](auto&... x) {
    return(ranges::views::concat(x.event_range()...));
  };

  /*
    cppcoro::generator<any_sir_event> test(){
    for(any_sir_event elem : std::get<0>(product_thing).event_range()){
    co_yield elem;
    }
    }
  */

  auto event_range() {
    return(std::apply(concat_lambda,product_thing));
  };

  single_time_event_generator() = default;
  single_time_event_generator(sir_state& initial_conditions) :
    product_thing(cartesian_multiproduct<event_index>(initial_conditions)...) {
  };
};

template<size_t event_index>
void main_func(sir_state& initial_conditions){
  size_t counter = 0;
  single_time_event_generator<number_of_event_types> gen2(initial_conditions);
  // single_time_event_generator<number_of_event_types> gen2;
  cartesian_multiproduct<event_index> gen(initial_conditions);
  auto event_range = gen2.event_range();
  // auto event_range = gen.event_range();
  for(auto elem : event_range){
    ++counter;
  }
  // std::cout << "finished iterating\n";
  // std::cout << "counter is " << counter << "\n";
};

int main(int argc, char** argv){

  if (argc != 3) {
    std::cout << "This program takes 2 runtimes arguments:\n\t- The number of people in the population\n\t- The number of time steps to run over\n";
    return -1;
  }
  size_t state_size = atoi(argv[1]);
  epidemic_time_t epidemic_length = atoi(argv[2]);
  std::cout << "Running " << epidemic_length << " time steps on a population of size " << state_size << "\n";

  std::string prefix = "main : ";
  sir_state initial_conditions = default_state(state_size);
  for(size_t elem : ranges::views::iota(0,epidemic_length)){
    if (elem < state_size){
      initial_conditions.potential_states[elem][I] = true;
    }
    // main_func<0>(initial_conditions);
    // main_func<1>(initial_conditions);
    main_func<2>(initial_conditions);

    // main_func(initial_conditions);
  }

}

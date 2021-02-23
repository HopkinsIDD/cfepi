#include <iostream>
#include <range/v3/all.hpp>
#include "/home/jkaminsky/git/cfepi/take_2/cfepi/include/sir.h"

template<size_t event_index>
void main_func(sir_state& initial_conditions){}

template<>
void main_func<0>(sir_state& initial_conditions){
  auto count_gen = ranges::span(initial_conditions.potential_states);
  unsigned long long counter = 0;
  auto range_iterator = ranges::views::cartesian_product();
  for(auto elem1 : range_iterator){
    ++counter;
  }
  // std::cout << "counter is " << counter << "\n";
  return;
}

template<>
void main_func<1>(sir_state& initial_conditions){
  auto count_gen = ranges::span(initial_conditions.potential_states);
  unsigned long long counter = 0;
  auto vec_2 = ranges::to<std::vector>(
    ranges::filter_view(
      ranges::span(initial_conditions.potential_states),
      [](auto && x){
	for(auto elem : {1}){
	  if(x[elem]){
	    return true;
	  }
	}
	return false;
      }
    )
  );
  auto range_iterator = ranges::views::cartesian_product(vec_2);
  for(auto elem1 : range_iterator){
    ++counter;
  }
  // std::cout << "counter is " << counter << "\n";
  return;
}

template<>
void main_func<2>(sir_state& initial_conditions){
  auto count_gen = ranges::span(initial_conditions.potential_states);
  size_t counter = 0;

  auto filtered_vectors = std::make_tuple(
    ranges::to<std::vector>(
      ranges::filter_view(
        ranges::span(initial_conditions.potential_states),
        [](auto && x){
  	for(auto elem : {0}){
  	  if(x[elem]){
  	    return true;
  	  }
  	}
  	return false;
        }
      )
    ),
    ranges::to<std::vector>(
      ranges::filter_view(
        ranges::span(initial_conditions.potential_states),
        [](auto && x){
  	for(auto elem : {1}){
  	  if(x[elem]){
  	    return true;
  	  }
  	}
  	return false;
        }
      )
    )
  );

  auto range_iterator = ranges::views::cartesian_product(std::get<0>(filtered_vectors), std::get<1>(filtered_vectors));
  for(auto elem1 : range_iterator){
    ++counter;
  }
  // std::cout << "counter is " << counter << "\n";
  return;
}


int main(int argc, char** argv){

  if (argc != 3) {
    std::cout << "This program takes 2 runtimes arguments:\n\t- The number of people in the population\n\t- The number of time steps to run over\n";
    return -1;
  }
  size_t state_size = atoi(argv[1]);
  epidemic_time_t epidemic_length = atoi(argv[2]);
  auto initial_conditions = default_state(state_size);
  for(auto elem : ranges::views::iota(0,epidemic_length)){
    if (elem < state_size){
      initial_conditions.potential_states[elem][I] = true;
    }
    main_func<0>(initial_conditions);
    main_func<1>(initial_conditions);
    main_func<2>(initial_conditions);
  }

}

#include <cfepi/sir.hpp>
#include <daw/daw_read_file.h>
#include <daw/json/daw_json_link.h>

namespace cfepi {

template<size_t num_states> struct config_epidemic_states {
public:
  using state = size_t;
  const std::array<std::string_view, num_states> state_array;
  constexpr static auto size() { return (num_states); };
  constexpr size_t operator[](std::string_view s) const {
    auto the_lambda = [s](const std::string_view x) { return (s == x); };

    return (static_cast<size_t>(std::distance(std::begin(state_array),
      std::find_if(std::begin(state_array), std::end(state_array), the_lambda))));
  }
  constexpr config_epidemic_states() : state_array(){};
  constexpr explicit config_epidemic_states(std::array<std::string_view, num_states> state_array_)
    : state_array(state_array_){};
};

}// namespace cfepi

namespace daw::json {
constexpr auto parse_json_select(std::string_view json, std::string_view elem) {
  return (
    std::get<1>(daw::json::json_details::find_range<NoCommentSkippingPolicyChecked>(json, elem)));
}
template<typename T> constexpr size_t parse_json_array_size(std::string_view json) {
  const auto vec = from_json_array<T>(json);
  return (vec.size());
};

template<typename T, size_t count> constexpr auto parse_json_array_values(std::string_view json) {
  const auto vec = from_json_array<T>(json);
  std::array<T, count> arr;
  std::copy(std::begin(vec), std::end(vec), std::begin(arr));
  return (arr);
};

constexpr auto parse_json_event_type_size(std::string_view json) {
  std::string_view sub_json = std::get<1>(
    daw::json::json_details::find_range<NoCommentSkippingPolicyChecked>(json, "source"));
  return (parse_json_array_size<json_delayed<no_name, std::string_view>>(sub_json));
};

template<typename states_t>
requires cfepi::is_sized_enum<states_t>
constexpr auto parse_json_event_type_source(states_t states, std::string_view json) {
  size_t bitset_int = 0UL;
  auto source_arr = from_json_array<std::string_view>(json);
  auto inner_lambda = [&states, &bitset_int](const auto &this_state) {
    bitset_int = bitset_int + (1UL << states[this_state]);
  };
  std::ranges::for_each(source_arr, inner_lambda);
  return (std::bitset<std::size(states_t{})>{ bitset_int });
};

template<typename states_t, size_t event_type_size>
requires cfepi::is_sized_enum<states_t>
constexpr auto parse_json_event_type_all_sources(states_t states, std::string_view json) {
  std::array<std::bitset<std::size(states_t{})>, event_type_size> rc{};
  std::vector<std::string_view> sources_vec{
    from_json_array<json_delayed<no_name, std::string_view>>(json)
  };
  std::transform(
    std::begin(sources_vec), std::end(sources_vec), std::begin(rc), [&states](const auto &x) {
      return (parse_json_event_type_source<states_t>(states, x));
    });
  return (rc);
};

template<typename states_t, size_t event_type_size>
requires cfepi::is_sized_enum<states_t>
constexpr auto parse_json_event_type_destination(states_t states, std::string_view json) {
  std::array<std::optional<typename states_t::state>, event_type_size> rc{};
  std::vector<std::string_view> destination_vec{
    from_json_array<json_delayed<no_name, std::string_view>>(json)
  };
  size_t destination_size{ std::size(destination_vec) };

  std::vector<std::string_view> compartments{};
  compartments.resize(destination_size);
  std::transform(std::begin(destination_vec),
    std::end(destination_vec),
    std::begin(compartments),
    [](std::string_view x) { return (from_json<std::string_view>(x, "compartment")); });

  std::vector<size_t> indices{};
  indices.resize(destination_size);
  std::transform(std::begin(destination_vec),
    std::end(destination_vec),
    std::begin(indices),
    [](std::string_view x) { return (from_json<size_t>(x, "index")); });

  std::vector<std::string_view> rc_tmp __attribute__((unused)){};
  std::ranges::for_each(std::ranges::views::iota(0UL, std::size(indices)),
    [&states, &rc, &indices, &compartments](
      const auto &index) { rc[indices[index]] = std::make_optional(states[compartments[index]]); });
  return (rc);
};

template<typename states_t, size_t event_type_size>
requires cfepi::is_sized_enum<states_t>
constexpr cfepi::sir_event_type<states_t, event_type_size> parse_json_event_type(states_t states,
  std::string_view json) {

  std::string_view source_json = std::get<1>(
    daw::json::json_details::find_range<NoCommentSkippingPolicyChecked>(json, "source"));
  std::string_view destination_json = std::get<1>(
    daw::json::json_details::find_range<NoCommentSkippingPolicyChecked>(json, "destination"));
  return (cfepi::sir_event_type<states_t, event_type_size>{
    parse_json_event_type_all_sources<states_t, event_type_size>(states, source_json),
    parse_json_event_type_destination<states_t, event_type_size>(states, destination_json) });
};

template<typename states_t, size_t num_events>
constexpr std::array<size_t, num_events> parse_json_array_event_type_sizes(std::string_view json) {
  std::array<size_t, num_events> rc{};
  std::vector<std::string_view> events_vec{
    from_json_array<json_delayed<no_name, std::string_view>>(json)
  };
  std::transform(std::begin(events_vec), std::end(events_vec), std::begin(rc), [](const auto &x) {
    return (parse_json_event_type_size(x));
  });
  return (rc);
}

template<typename double_type, size_t num_events>
constexpr std::array<double_type, num_events> parse_json_array_event_type_probabilities(
  std::string_view json) {
  std::array<double_type, num_events> rc{};
  std::vector<std::string_view> events_vec{
    from_json_array<json_delayed<no_name, std::string_view>>(json)
  };
  std::transform(std::begin(events_vec), std::end(events_vec), std::begin(rc), [](auto &x) {
    return (from_json<double_type>(x, "probability"));
  });
  return (rc);
}

template<typename states_t>
// requires std::same_as<states_t, cfepi::config_epidemic_states>
constexpr cfepi::sir_state<states_t> parse_json_initial_conditions(states_t states, std::string_view json) {

  size_t counter{0};
  for(auto compartment_name : states.state_array) {
    try {
      auto initial_count_for_this_compartment = from_json<cfepi::person_t>(json, compartment_name);
      counter += initial_count_for_this_compartment;
    } catch (daw::json::json_exception const &jex) {
      if (jex.reason() == "JSON Path specified not found in document") {
	std::cerr << "No initial conditions provided for state " << compartment_name << "\n";
      } else {
	throw jex;
      }
    }
  }
  cfepi::sir_state<states_t> rc{counter};

  counter = 0UL;
  for(auto compartment_name : states.state_array) {
    try {
      auto initial_count_for_this_compartment = from_json<cfepi::person_t>(json, compartment_name);
      for(auto index : std::ranges::views::iota(counter, counter + initial_count_for_this_compartment)) {
	rc.potential_states[index] = std::bitset<std::size(states_t{})>{(1UL << states[compartment_name])};
      }
    counter += initial_count_for_this_compartment;
    } catch (daw::json::json_exception const &jex) {
      // This should have already happened
    }
  }
  return (rc);
}

template<typename states_t,
  auto event_sizes,
  typename = std::make_index_sequence<std::size(event_sizes)>>
struct parse_json_array_event_type_struct;

template<typename states_t, auto event_sizes, size_t... event_sizes_index>
struct parse_json_array_event_type_struct<states_t,
  event_sizes,
  std::index_sequence<event_sizes_index...>> {
  constexpr std::tuple<cfepi::sir_event_type<states_t, event_sizes[event_sizes_index]>...>
    parse(states_t states, std::string_view json) {
    auto sub_jsons = from_json_array<json_delayed<no_name, std::string_view>>(json);
    auto rc = std::make_tuple(parse_json_event_type<states_t, event_sizes[event_sizes_index]>(
      states, sub_jsons[event_sizes_index])...);
    return (rc);
  }
  typedef std::variant<cfepi::sir_event_type<states_t, event_sizes[event_sizes_index]>...>
    any_event_type;
};


}// namespace daw::json


template<auto arr> void print_first_element() { std::cout << arr[0] << "\n"; }

namespace daw::json {}// namespace daw::json

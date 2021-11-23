#include <catch2/catch.hpp>

#include <cfepi/sir.hpp>
TEST_CASE("Repeat works","[repeat]")
{
  STATIC_REQUIRE(std::is_same<repeat<int,1>, int>::value );
  STATIC_REQUIRE(std::is_same<repeat<int,2>, int>::value );
  STATIC_REQUIRE(std::is_same<repeat<int,3>, int>::value );
}

TEST_CASE("Sized Enum concept works", "[sir_state]")
{

  struct simple_enum{
  public:
    enum state {A, B, C, D, E, state_count};
    constexpr auto size() const {
      return(static_cast<size_t>(state_count));
    }
  };

  constexpr bool simple_enum_is_sized_enum = is_sized_enum<simple_enum>;
  STATIC_REQUIRE(simple_enum_is_sized_enum);

  auto x = sir_state<simple_enum>{1UL}.potential_states[0];
  STATIC_REQUIRE(std::is_same<decltype(x),std::bitset<simple_enum::state_count> >::value);
  sir_state<simple_enum> test_state{1UL};
  test_state.potential_states[0][simple_enum::D] = true;
  test_state.potential_states[0][simple_enum::D] = true;
}

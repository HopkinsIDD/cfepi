#include <catch2/catch.hpp>

#include <cfepi/sir.hpp>
TEST_CASE("Repeat works","[repeat]")
{
  STATIC_REQUIRE(std::is_same<repeat<int,1>, int>::value );
  STATIC_REQUIRE(std::is_same<repeat<int,2>, int>::value );
  STATIC_REQUIRE(std::is_same<repeat<int,3>, int>::value );
}

TEST_CASE("Default State Works", "[sir_state]")
{
  STATIC_REQUIRE(std::is_same<decltype(default_state()), sir_state>::value);
}

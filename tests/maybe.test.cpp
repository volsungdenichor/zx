#include <catch2/catch_test_macros.hpp>
#include <zx/zx.hpp>

TEST_CASE("default constructed maybe has no value", "[monadic][maybe]")
{
    REQUIRE_FALSE(zx::maybe<int>{}.has_value());
}

TEST_CASE("default constructed maybe has converts to false", "[monadic][maybe]")
{
    REQUIRE_FALSE(zx::maybe<int>{});
}

TEST_CASE("default constructed with equal type has value", "[monadic][maybe]")
{
    REQUIRE(zx::maybe<int>{ 42 }.has_value());
}

TEST_CASE("default constructed with equal type converts to true", "[monadic][maybe]")
{
    REQUIRE(zx::maybe<int>{ 42 });
}

TEST_CASE("extracting value from value initialized maybe returns proper result", "[monadic][maybe]")
{
    REQUIRE(*zx::maybe<int>{ 42 } == 42);
    REQUIRE(zx::maybe<int>{ 42 }.value() == 42);
}

// TEST_CASE("extracting value from empty maybe returns throws exception", "[monadic][maybe]")
// {
//     REQUIRE_THROWS(*zx::maybe<int>{});
//     REQUIRE_THROWS(zx::maybe<int>{}.value());
// }

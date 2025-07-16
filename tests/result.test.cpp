#include <catch2/catch_test_macros.hpp>
#include <zx/zx.hpp>

using error_type = std::string;

TEST_CASE("default constructed result has value", "[monadic][result]")
{
    REQUIRE(zx::result<int, error_type>{}.has_value());
    REQUIRE(zx::result<void, error_type>{}.has_value());
}

TEST_CASE("result has value", "[monadic][result]")
{
    const auto res = zx::result<int, error_type>{ 42 };
    REQUIRE(res.has_value());
    REQUIRE_FALSE(res.has_error());
    REQUIRE(res.value() == 42);
    REQUIRE(*res == 42);
}

TEST_CASE("void result has value", "[monadic][result]")
{
    const auto res = zx::result<void, error_type>{};
    REQUIRE(res.has_value());
    REQUIRE_FALSE(res.has_error());
}

TEST_CASE("result with error has error", "[monadic][result]")
{
    const auto res = zx::result<int, error_type>{ zx::error(error_type{ "this-is-not-ok" }) };
    REQUIRE_FALSE(res.has_value());
    REQUIRE(res.has_error());
    REQUIRE(res.error() == error_type{ "this-is-not-ok" });
}

TEST_CASE("void result with error has error", "[monadic][result]")
{
    const auto res = zx::result<void, error_type>{ zx::error(error_type{ "this-is-not-ok" }) };
    REQUIRE_FALSE(res.has_value());
    REQUIRE(res.has_error());
    REQUIRE(res.error() == error_type{ "this-is-not-ok" });
}

TEST_CASE("transform - value to value", "[monadic][result]")
{
    const auto res = zx::result<int, error_type>{ 42 }.transform(zx::str);
    REQUIRE(std::is_same_v<std::decay_t<decltype(res)>, zx::result<std::string, error_type>>);
    REQUIRE(res.has_value());
    REQUIRE_FALSE(res.has_error());
    REQUIRE(res.value() == "42");
}

TEST_CASE("transform - error to value", "[monadic][result]")
{
    const auto res = zx::result<int, error_type>{ zx::error(error_type{ "this-is-not-ok" }) }.transform(zx::str);
    REQUIRE(std::is_same_v<std::decay_t<decltype(res)>, zx::result<std::string, error_type>>);
    REQUIRE_FALSE(res.has_value());
    REQUIRE(res.has_error());
    REQUIRE(res.error() == error_type{ "this-is-not-ok" });
}

TEST_CASE("transform - void with value to value", "[monadic][result]")
{
    const auto res = zx::result<void, error_type>{}.transform([]() -> std::string { return "@@@"; });
    REQUIRE(std::is_same_v<std::decay_t<decltype(res)>, zx::result<std::string, error_type>>);
    REQUIRE(res.has_value());
    REQUIRE_FALSE(res.has_error());
    REQUIRE(res.value() == "@@@");
}

TEST_CASE("transform - void with value to void", "[monadic][result]")
{
    const auto res = zx::result<void, error_type>{}.transform([]() {});
    REQUIRE(std::is_same_v<std::decay_t<decltype(res)>, zx::result<void, error_type>>);
    REQUIRE(res.has_value());
    REQUIRE_FALSE(res.has_error());
}

TEST_CASE("transform - void with error to void", "[monadic][result]")
{
    const auto res = zx::result<void, error_type>{ zx::error(error_type{ "this-is-not-ok" }) }.transform([]() {});
    REQUIRE(std::is_same_v<std::decay_t<decltype(res)>, zx::result<void, error_type>>);
    REQUIRE_FALSE(res.has_value());
    REQUIRE(res.has_error());
    REQUIRE(res.error() == error_type{ "this-is-not-ok" });
}

TEST_CASE("and_then - value to value", "[monadic][result]")
{
    const auto res = zx::result<int, error_type>{ 42 }.and_then(
        [](int x) -> zx::result<std::string, error_type> { return zx::str(x / 2); });
    REQUIRE(std::is_same_v<std::decay_t<decltype(res)>, zx::result<std::string, error_type>>);
    REQUIRE(res.has_value());
    REQUIRE_FALSE(res.has_error());
    REQUIRE(res.value() == "21");
}

TEST_CASE("and_then - value to error", "[monadic][result]")
{
    const auto res = zx::result<int, error_type>{ 42 }.and_then(
        [](int x) -> zx::result<std::string, error_type> { return zx::error(error_type{ "not-good" }); });
    REQUIRE(std::is_same_v<std::decay_t<decltype(res)>, zx::result<std::string, error_type>>);
    REQUIRE_FALSE(res.has_value());
    REQUIRE(res.has_error());
    REQUIRE(res.error() == error_type{ "not-good" });
}

TEST_CASE("and_then - value with error to value", "[monadic][result]")
{
    const auto res = zx::result<int, error_type>{ zx::error(error_type{ "not-good" }) }.and_then(
        [](int x) -> zx::result<std::string, error_type> { return zx::str(x / 2); });
    REQUIRE(std::is_same_v<std::decay_t<decltype(res)>, zx::result<std::string, error_type>>);
    REQUIRE_FALSE(res.has_value());
    REQUIRE(res.has_error());
    REQUIRE(res.error() == error_type{ "not-good" });
}

TEST_CASE("and_then - value with error to error", "[monadic][result]")
{
    const auto res = zx::result<int, error_type>{ zx::error(error_type{ "not-good" }) }.and_then(
        [](int x) -> zx::result<std::string, error_type> { return zx::error(error_type{ "even-worse" }); });
    REQUIRE(std::is_same_v<std::decay_t<decltype(res)>, zx::result<std::string, error_type>>);
    REQUIRE_FALSE(res.has_value());
    REQUIRE(res.has_error());
    REQUIRE(res.error() == error_type{ "not-good" });
}

TEST_CASE("and_then - void to value", "[monadic][result]")
{
    const auto res = zx::result<void, error_type>{}.and_then(
        []() -> zx::result<std::string, error_type> { return zx::error(error_type{ "even-worse" }); });
    REQUIRE(std::is_same_v<std::decay_t<decltype(res)>, zx::result<std::string, error_type>>);
    REQUIRE_FALSE(res.has_value());
    REQUIRE(res.has_error());
    REQUIRE(res.error() == error_type{ "even-worse" });
}

TEST_CASE("and_then - void with error to error", "[monadic][result]")
{
    const auto res = zx::result<void, error_type>{ zx::error(error_type{ "not-good" }) }.and_then(
        []() -> zx::result<std::string, error_type> { return zx::error(error_type{ "even-worse" }); });
    REQUIRE(std::is_same_v<std::decay_t<decltype(res)>, zx::result<std::string, error_type>>);
    REQUIRE_FALSE(res.has_value());
    REQUIRE(res.has_error());
    REQUIRE(res.error() == error_type{ "not-good" });
}

TEST_CASE("and_then - void with error to value", "[monadic][result]")
{
    const auto res = zx::result<void, error_type>{ zx::error(error_type{ "not-good" }) }.and_then(
        []() -> zx::result<std::string, error_type> { return std::string{ "@@" }; });
    REQUIRE(std::is_same_v<std::decay_t<decltype(res)>, zx::result<std::string, error_type>>);
    REQUIRE_FALSE(res.has_value());
    REQUIRE(res.has_error());
    REQUIRE(res.error() == error_type{ "not-good" });
}

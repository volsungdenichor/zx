#include <zx/zx.hpp>

#include "matchers.hpp"

TEST_CASE("reduce", "")
{
    REQUIRE_THAT(zx::reduce(0, std::plus<>{})(std::vector{ 1, 3, 5, 100 }), matchers::equal_to(109));
    REQUIRE_THAT(
        zx::reduce(
            std::string{},
            [](std::string s, int v) -> std::string
            { return s.empty() ? std::to_string(v) : s + ", " + std::to_string(v); })(std::vector{ 1, 3, 5, 100 }),
        matchers::equal_to("1, 3, 5, 100"));
}

TEST_CASE("reduce - multiple input", "")
{
    REQUIRE_THAT(zx::reduce(0, std::plus<>{})(std::vector{ 1, 3, 5, 100 }), matchers::equal_to(109));
    REQUIRE_THAT(
        zx::reduce(
            std::string{},
            [](std::string s, int v, char ch) -> std::string
            { return ch == '+' ? s + std::to_string(v) : s; })(std::vector{ 1, 3, 5, 100 }, std::string("++--")),
        matchers::equal_to("13"));

    REQUIRE_THAT(
        zx::reduce(0, [](int acc, int lt, int rt) -> int { return acc + lt * rt; })(
            std::vector{ 1, 3, 5 }, std::vector{ 10, 20, 5 }),
        matchers::equal_to(95));
}

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

TEST_CASE("deconstruct", "")
{
    REQUIRE_THAT(
        (std::tuple{ 2, 3, 4 } |= zx::deconstruct([](int x, int y, int z) { return x * (y + z); })), matchers::equal_to(14));
}

TEST_CASE("let", "")
{
    REQUIRE_THAT(zx::let([]() { return 3; }), matchers::equal_to(3));
    REQUIRE_THAT(zx::let(2 + 3, [](int x) { return 3 * x; }), matchers::equal_to(15));
    REQUIRE_THAT(
        zx::let(2 + 3, "!", [](int x, const std::string& y) { return zx::str(x, '.', y); }), matchers::equal_to("5.!"));
    REQUIRE_THAT(
        zx::let(2 + 3, "!", 'x', [](int x, const std::string& y, char z) { return zx::str(x, '.', y, '.', z); }),
        matchers::equal_to("5.!.x"));
    REQUIRE_THAT(
        zx::let(
            std::vector<int>{ 2, 5, 13 },
            std::tuple{ ", ", "[", "]" },
            [](const auto& vect, const auto& ctrl) -> std::string
            {
                const auto& [sep, open, close] = ctrl;
                std::stringstream ss;
                ss << open;
                for (std::size_t i = 0; i < vect.size(); ++i)
                {
                    if (i != 0)
                    {
                        ss << sep;
                    }
                    ss << vect[i];
                }
                ss << close;
                return ss.str();
            }),
        matchers::equal_to("[2, 5, 13]"));
}

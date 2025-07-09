#include <zx/zx.hpp>

#include "matchers.hpp"

template <class Func>
constexpr auto associate(Func func)
{
    return [=](auto&& item)
    {
        auto&& res = std::invoke(func, item);
        return std::pair{ std::forward<decltype(item)>(item), res };
    };
}

TEST_CASE("sequence - create from container", "[sequence]")
{
    std::vector<int> v = { 1, 1, 2, 3, 5, 8, 13 };
    REQUIRE_THAT(zx::sequence<int>(v), matchers::elements_are(1, 1, 2, 3, 5, 8, 13));
}

TEST_CASE("sequence - range", "[sequence]")
{
    REQUIRE_THAT(zx::seq::range(5), matchers::elements_are(0, 1, 2, 3, 4));
    REQUIRE_THAT(zx::seq::range(5, 10), matchers::elements_are(5, 6, 7, 8, 9));
}

TEST_CASE("sequence - transform", "[sequence][transform]")
{
    REQUIRE_THAT(zx::seq::range(5).transform([](int x) { return x * x; }), matchers::elements_are(0, 1, 4, 9, 16));
    REQUIRE_THAT(
        zx::seq::range(5).transform(associate([](int x) { return x * x; })),
        matchers::elements_are(
            std::pair{ 0, 0 }, std::pair{ 1, 1 }, std::pair{ 2, 4 }, std::pair{ 3, 9 }, std::pair{ 4, 16 }));
}

TEST_CASE("sequence - filter", "[sequence][filter]")
{
    REQUIRE_THAT(zx::seq::range(5).filter([](int x) { return x % 2 == 0; }), matchers::elements_are(0, 2, 4));
}

TEST_CASE("sequence - slice", "[sequence][slicing]")
{
    REQUIRE_THAT(zx::seq::range(10).slice({ 1, 4 }), matchers::elements_are(1, 2, 3));
    REQUIRE_THAT(zx::seq::range(10).slice({ {}, 4 }), matchers::elements_are(0, 1, 2, 3));
    REQUIRE_THAT(zx::seq::range(10).slice({ 4, {} }), matchers::elements_are(4, 5, 6, 7, 8, 9));
}

TEST_CASE("sequence - associate", "[sequence][transform]")
{
    const std::map<int, std::string> map
        = zx::seq::range(5).transform(associate([](int x) { return zx::str('|', x * (x + 2), '|'); }));
    REQUIRE_THAT(map, matchers::size_is(5));
    REQUIRE_THAT(map.at(0), matchers::equal_to("|0|"));
    REQUIRE_THAT(map.at(1), matchers::equal_to("|3|"));
    REQUIRE_THAT(map.at(2), matchers::equal_to("|8|"));
    REQUIRE_THAT(map.at(3), matchers::equal_to("|15|"));
    REQUIRE_THAT(map.at(4), matchers::equal_to("|24|"));
}

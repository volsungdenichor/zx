#include <zx/zx.hpp>

#include "matchers.hpp"

template <class Func>
constexpr auto associate(Func func)
{
    return [=](auto&& item)
    {
        auto&& res = std::invoke(func, item);
        return std::tuple{ std::forward<decltype(item)>(item), res };
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
            std::tuple{ 0, 0 }, std::tuple{ 1, 1 }, std::tuple{ 2, 4 }, std::tuple{ 3, 9 }, std::tuple{ 4, 16 }));
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

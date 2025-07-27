#include <zx/zx.hpp>

#include "matchers.hpp"

TEST_CASE("iterator_range - init span from std::vector", "")
{
    constexpr auto sum = [](zx::span<int> s) { return zx::reduce(0, std::plus<>{})(s); };
    std::vector<int> vect = { 2, 3, 5, 9 };
    REQUIRE_THAT(sum(vect), matchers::equal_to(19));
}

TEST_CASE("iterator_range - slicing", "")
{
    std::vector<int> vect = { 1, 2, 3, 4, 5, 6, 7 };
    REQUIRE_THAT(zx::span<int>{ vect }, matchers::elements_are(1, 2, 3, 4, 5, 6, 7));
    REQUIRE_THAT(zx::span<int>{ vect }.take(4), matchers::elements_are(1, 2, 3, 4));
    REQUIRE_THAT(zx::span<int>{ vect }.drop(4), matchers::elements_are(5, 6, 7));
    REQUIRE_THAT(zx::span<int>{ vect }.take_back(4), matchers::elements_are(4, 5, 6, 7));
    REQUIRE_THAT(zx::span<int>{ vect }.drop_back(4), matchers::elements_are(1, 2, 3));
    REQUIRE_THAT(zx::span<int>{ vect }.slice({ {}, 3 }), matchers::elements_are(1, 2, 3));
    REQUIRE_THAT(zx::span<int>{ vect }.slice({ 0, 3 }), matchers::elements_are(1, 2, 3));
    REQUIRE_THAT(zx::span<int>{ vect }.slice({ 1, 3 }), matchers::elements_are(2, 3));
    REQUIRE_THAT(zx::span<int>{ vect }.slice({ 3, {} }), matchers::elements_are(4, 5, 6, 7));
    REQUIRE_THAT(zx::span<int>{ vect }.slice({ 3, 5 }), matchers::elements_are(4, 5));
    REQUIRE_THAT(zx::span<int>{ vect }.slice({ -2, {} }), matchers::elements_are(6, 7));
    REQUIRE_THAT(zx::span<int>{ vect }.slice({ {}, -2 }), matchers::elements_are(1, 2, 3, 4, 5));
    REQUIRE_THAT(zx::span<int>{ vect }.slice({ -4, -2 }), matchers::elements_are(4, 5));
    REQUIRE_THAT(zx::span<int>{ vect }.slice({ -2, -4 }), matchers::is_empty());
}

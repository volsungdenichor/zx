#include <zx/zx.hpp>

#include "matchers.hpp"

struct iter_impl
{
    int m_value;

    explicit iter_impl(int value = 0) : m_value(value)
    {
    }

    auto deref() const -> int
    {
        return m_value;
    }

    void advance(std::ptrdiff_t offset)
    {
        m_value += 10 * offset;
    }

    auto distance_to(const iter_impl& other) const -> std::ptrdiff_t
    {
        return (other.m_value - m_value) / 10;
    }
};

using test_iterator = zx::iterator_interface<iter_impl>;

TEST_CASE("iterator_interface", "[iterator]")
{
    REQUIRE(std::is_same_v<std::random_access_iterator_tag, zx::iter_category_t<test_iterator>>);
    REQUIRE(zx::is_random_access_iterator<test_iterator>::value);
}

TEST_CASE("iterator_interface - advance", "[iterator]")
{
    const auto iter = test_iterator{ 10 };
    REQUIRE_THAT(*iter, matchers::equal_to(10));
    REQUIRE_THAT(iter[1], matchers::equal_to(20));
}

TEST_CASE("iterator_interface - distance", "[iterator]")
{
    const auto lhs = test_iterator{ 10 };
    const auto rhs = test_iterator{ 50 };
    REQUIRE_THAT(std::distance(lhs, rhs), matchers::equal_to(4));
}

TEST_CASE("iterator_interface - values", "[iterator]")
{
    REQUIRE_THAT(
        std::accumulate(
            test_iterator{ 10 },
            test_iterator{ 50 },
            std::vector<int>{},
            [](std::vector<int> acc, int v) -> std::vector<int>
            {
                acc.push_back(v);
                return acc;
            }),
        matchers::elements_are(10, 20, 30, 40));
}

TEST_CASE("iterator_interface - numeric range", "[iterator]")
{
    REQUIRE_THAT(zx::range(5, 9), matchers::elements_are(5, 6, 7, 8));
    REQUIRE_THAT(zx::range(5), matchers::elements_are(0, 1, 2, 3, 4));
    REQUIRE_THAT(zx::range(5).reverse(), matchers::elements_are(4, 3, 2, 1, 0));
    REQUIRE_THAT(zx::iota().take(5), matchers::elements_are(0, 1, 2, 3, 4));
    REQUIRE_THAT(zx::iota(5).take(5), matchers::elements_are(5, 6, 7, 8, 9));

    REQUIRE_THAT(zx::range(9, 5), matchers::is_empty());

    REQUIRE(std::is_same_v<std::random_access_iterator_tag, zx::range_category_t<decltype(zx::range(2))>>);

    const auto rng = zx::range(5, 9);
    const auto b = std::begin(rng);
    const auto e = std::end(rng);
    REQUIRE_THAT(std::distance(b, e), matchers::equal_to(4));
}

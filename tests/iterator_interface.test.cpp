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

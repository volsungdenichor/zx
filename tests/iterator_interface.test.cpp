#include <zx/zx.hpp>

#include "matchers.hpp"

struct iter_impl
{
    int m_value;

    iter_impl(int value = 0) : m_value(value)
    {
    }

    int deref() const
    {
        return m_value;
    }

    void advance(std::ptrdiff_t offset)
    {
        m_value += 10 * offset;
    }

    std::ptrdiff_t distance_to(const iter_impl& other) const
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
    REQUIRE(*iter == 10);
    REQUIRE(iter[1] == 20);
}

TEST_CASE("iterator_interface - distance", "[iterator]")
{
    const auto lhs = test_iterator{ 10 };
    const auto rhs = test_iterator{ 50 };
    REQUIRE(std::distance(lhs, rhs) == 4);
}

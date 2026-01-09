#include <gmock/gmock.h>

#include <zx/iterator_interface.hpp>

namespace
{

struct random_access_iter_impl
{
    int m_value;

    explicit random_access_iter_impl(int value = 0) : m_value(value)
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

    auto distance_to(const random_access_iter_impl& other) const -> std::ptrdiff_t
    {
        return (other.m_value - m_value) / 10;
    }
};

using random_access_iter = zx::iterator_interface<random_access_iter_impl>;

struct forward_iter_impl
{
    int m_value;

    explicit forward_iter_impl(int value = 0) : m_value(value)
    {
    }

    auto deref() const -> int
    {
        return m_value;
    }

    void inc()
    {
        m_value += 1;
    }

    bool is_equal(const forward_iter_impl& other) const
    {
        return m_value == other.m_value;
    }
};

using forward_iter = zx::iterator_interface<forward_iter_impl>;

}  // namespace

TEST(iterator_interface, random_access_iterator)
{
    EXPECT_THAT(zx::is_random_access_iterator<random_access_iter>::value, testing::IsTrue());

    random_access_iter it1{ 0 };
    random_access_iter it2{ 20 };

    EXPECT_THAT(*it1, 0);
    EXPECT_THAT(*it2, 20);

    EXPECT_THAT(it1[0], 0);
    EXPECT_THAT(it1[1], 10);

    ++it1;
    EXPECT_THAT(*it1, 10);

    it1++;
    EXPECT_THAT(*it1, 20);

    EXPECT_THAT(it1 == it2, true);
    EXPECT_THAT(it1 != it2, false);
    EXPECT_THAT(it1 <= it2, true);
    EXPECT_THAT(it1 >= it2, true);
    EXPECT_THAT(it1 < it2, false);
    EXPECT_THAT(it1 > it2, false);

    std::ptrdiff_t dist = it2 - it1;
    EXPECT_THAT(dist, 0);

    --it1;
    EXPECT_THAT(*it1, 10);

    it1--;
    EXPECT_THAT(*it1, 0);
}

TEST(iterator_interface, forward_iterator)
{
    EXPECT_THAT(zx::is_forward_iterator<forward_iter>::value, testing::IsTrue());

    forward_iter it{ 0 };

    EXPECT_THAT(*it, 0);

    ++it;
    EXPECT_THAT(*it, 1);

    it++;
    EXPECT_THAT(*it, 2);
}

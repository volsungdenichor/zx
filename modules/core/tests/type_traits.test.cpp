#include <gmock/gmock.h>

#include <forward_list>
#include <iterator>
#include <vector>
#include <zx/type_traits.hpp>

namespace
{

template <class T>
using has_value_type_impl = typename T::value_type;

template <class T>
using has_value_type = zx::is_detected<has_value_type_impl, T>;

template <class T>
using is_incrementable_impl = decltype(++std::declval<T&>());

template <class T>
struct is_incrementable : zx::is_detected<is_incrementable_impl, T>
{
};

template <class T>
using is_decrementable_impl = decltype(--std::declval<T&>());

template <class T>
struct is_decrementable : zx::is_detected<is_decrementable_impl, T>
{
};

template <class T>
struct is_incrementable_decrementable : std::conjunction<is_incrementable<T>, is_decrementable<T>>
{
};

}  // namespace

TEST(type_traits, is_detected_with_internal_typedef)
{
    struct with_value_type
    {
        using value_type = int;
    };
    struct without_value_type
    {
    };

    EXPECT_THAT((has_value_type<with_value_type>::value), testing::IsTrue());
    EXPECT_THAT((has_value_type<without_value_type>::value), testing::IsFalse());
}

TEST(type_traits, is_detected_with_incrementable)
{
    struct incrementable
    {
        incrementable& operator++() { return *this; }
    };

    struct non_incrementable
    {
    };

    EXPECT_THAT((is_incrementable<incrementable>::value), testing::IsTrue());
    EXPECT_THAT((is_incrementable<non_incrementable>::value), testing::IsFalse());
}

TEST(type_traits, is_detected_with_incrementable_decrementable)
{
    struct incrementable_decrementable
    {
        incrementable_decrementable& operator++() { return *this; }

        incrementable_decrementable& operator--() { return *this; }
    };

    struct incrementable_only
    {
        incrementable_only& operator++() { return *this; }
    };

    EXPECT_THAT((is_incrementable_decrementable<incrementable_decrementable>::value), testing::IsTrue());
    EXPECT_THAT((is_incrementable_decrementable<incrementable_only>::value), testing::IsFalse());
}

TEST(type_traits, is_random_access_iterator)
{
    EXPECT_THAT((zx::is_random_access_iterator<int*>::value), testing::IsTrue());
    EXPECT_THAT((zx::is_random_access_iterator<std::vector<int>::iterator>::value), testing::IsTrue());

    EXPECT_THAT((zx::is_random_access_iterator<std::forward_list<int>::iterator>::value), testing::IsFalse());
    EXPECT_THAT((zx::is_random_access_iterator<std::istream_iterator<int>>::value), testing::IsFalse());
}

TEST(type_traits, is_forward_iterator)
{
    EXPECT_THAT((zx::is_forward_iterator<int*>::value), testing::IsTrue());
    EXPECT_THAT((zx::is_forward_iterator<std::vector<int>::iterator>::value), testing::IsTrue());

    EXPECT_THAT((zx::is_forward_iterator<std::forward_list<int>::iterator>::value), testing::IsTrue());
    EXPECT_THAT((zx::is_random_access_iterator<std::istream_iterator<int>>::value), testing::IsFalse());
}

TEST(type_traits, is_input_iterator)
{
    EXPECT_THAT((zx::is_input_iterator<int*>::value), testing::IsTrue());
    EXPECT_THAT((zx::is_input_iterator<std::vector<int>::iterator>::value), testing::IsTrue());

    EXPECT_THAT((zx::is_input_iterator<std::forward_list<int>::iterator>::value), testing::IsTrue());
    EXPECT_THAT((zx::is_input_iterator<std::istream_iterator<int>>::value), testing::IsTrue());
}

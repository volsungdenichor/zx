#include <gmock/gmock.h>

#include <zx/functions.hpp>

TEST(functions, identity)
{
    using namespace std::literals;
    zx::identity_t identity;

    EXPECT_THAT(identity(42), 42);
    EXPECT_THAT(identity("Hello"s), "Hello"s);
}

TEST(functions, get_element)
{
    using namespace std::literals;
    const auto tuple = std::tuple{ 42, "Hello"s, 3.14 };

    EXPECT_THAT(zx::get_element<0>(tuple), 42);
    EXPECT_THAT(zx::get_element<1>(tuple), "Hello"s);
    EXPECT_THAT(zx::get_element<2>(tuple), 3.14);
}

TEST(functions, dereference)
{
    const int value = 42;
    EXPECT_THAT(zx::dereference(value), 42);
    testing::StaticAssertTypeEq<decltype(zx::dereference(value)), const int&>();

    std::optional<int> optValue = 42;
    EXPECT_THAT(zx::dereference(optValue), 42);
    testing::StaticAssertTypeEq<decltype(zx::dereference(optValue)), int&>();

    std::optional<std::reference_wrapper<const int>> optRef = value;
    EXPECT_THAT(zx::dereference(optRef), 42);
    testing::StaticAssertTypeEq<decltype(zx::dereference(optRef)), const int&>();

    std::optional<int> emptyOpt = {};
    EXPECT_THROW(zx::dereference(emptyOpt), std::runtime_error);
}

TEST(functions, proj)
{
    using namespace std::literals;
    const auto func = zx::proj(std::plus<>{}, &std::string::size);
    EXPECT_THAT(func("Hello"s, "World"s), 10);
    EXPECT_THAT(func("C++"s, "Programming"s), 14);
}

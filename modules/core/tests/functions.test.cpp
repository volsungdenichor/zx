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
    EXPECT_THAT(
        [&] { zx::dereference(emptyOpt); },
        testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("Attempted to dereference an empty value")));
}

TEST(functions, proj)
{
    using namespace std::literals;
    const auto func = zx::proj(std::plus<>{}, &std::string::size);
    EXPECT_THAT(func("Hello"s, "World"s), 10);
    EXPECT_THAT(func("C++"s, "Programming"s), 14);
}

TEST(functions, cast)
{
    const auto cast_to_int = zx::cast<int>;
    EXPECT_THAT(cast_to_int(42.5), 42);
    EXPECT_THAT(cast_to_int(3.14f), 3);
}

TEST(functions, overloaded)
{
    using namespace std::literals;
    const auto func = zx::overloaded(
        [](const auto& self, int x) -> int { return x >= 2 ? self(x - 1) + self(x - 2) : 1; },
        [](const auto&, double) -> int { return 666; },
        [](const auto& self, const std::string& s) -> int { return self(s.size()); });

    EXPECT_THAT(func(1), 1);
    EXPECT_THAT(func(2), 2);
    EXPECT_THAT(func(3), 3);
    EXPECT_THAT(func(4), 5);
    EXPECT_THAT(func(5), 8);
    EXPECT_THAT(func(6), 13);
    EXPECT_THAT(func(""s), 1);
    EXPECT_THAT(func("C++"s), 3);
    EXPECT_THAT(func("Hello"s), 8);
    EXPECT_THAT(func(3.14), 666);
}

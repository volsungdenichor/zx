#include <gmock/gmock.h>

#include <zx/functional.hpp>

namespace
{
constexpr auto uppercase = [](std::string& s)
{
    for (auto& c : s)
    {
        c = static_cast<char>(std::toupper(c));
    }
};
constexpr auto take = [](std::size_t n)
{
    return [n](std::string& s)
    {
        if (s.size() > n)
        {
            s.resize(n);
        }
    };
};
}  // namespace

TEST(functional, apply)
{
    std::string str = "hello";
    str |= zx::apply(uppercase, take(4));
    EXPECT_THAT(str, testing::Eq("HELL"));
}

TEST(functional, with)
{
    const std::string str = "hello";
    const auto result = str |= zx::with(uppercase, take(4));
    EXPECT_THAT(str, testing::Eq("hello"));
    EXPECT_THAT(result, testing::Eq("HELL"));
}

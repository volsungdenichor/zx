#include <gmock/gmock.h>

#include <zx/algorithm.hpp>

TEST(algorithm, fold_left)
{
    std::vector<int> numbers = { 1, 2, 3, 4, 5 };
    EXPECT_THAT(zx::fold_left(numbers, 0, [](int acc, int n) { return acc + n; }), testing::Eq(15));
}

TEST(algorithm, fold_left_with_different_result_type)
{
    std::vector<int> numbers = { 1, 2, 3, 4, 5 };
    EXPECT_THAT(
        zx::fold_left(numbers, "", [](std::string acc, int n) { return acc + std::to_string(n); }), testing::Eq("12345"));
}

TEST(algorithm, fold_left_first)
{
    std::vector<int> numbers = { 1, 2, 3, 4, 5 };
    EXPECT_THAT(zx::fold_left_first(numbers, [](int acc, int n) { return acc + n; }), testing::Optional(15));
    EXPECT_THAT(zx::fold_left_first(std::vector<int>{}, [](int acc, int n) { return acc + n; }), testing::Eq(std::nullopt));
}

TEST(algorithm, try_fold_left_returns_value_when_no_error)
{
    std::vector<int> numbers = { 1, 2, 3, 4, 5 };
    EXPECT_THAT(
        zx::try_fold_left(numbers, 0, [](int acc, int n) -> zx::result_t<int, std::string> { return acc + n; }),
        testing::Eq(zx::result_t<int, std::string>{ 15 }));
}

TEST(algorithm, try_fold_left_fails_on_error)
{
    std::vector<int> numbers = { 1, 2, 3, 4, 5 };
    EXPECT_THAT(
        zx::try_fold_left(
            numbers,
            0,
            [](int acc, int n) -> zx::result_t<int, std::string>
            {
                if (n == 3)
                {
                    return zx::error("Error at 3");
                }
                return acc + n;
            }),
        testing::Eq(zx::result_t<int, std::string>{ zx::error("Error at 3") }));
}

TEST(algorithm, try_fold_left_first)
{
    std::vector<int> numbers = { 1, 2, 3, 4, 5 };
    EXPECT_THAT(
        zx::try_fold_left_first(numbers, [](int acc, int n) -> zx::result_t<int, std::string> { return acc + n; }),
        testing::Eq(zx::result_t<zx::maybe_t<int>, std::string>{ zx::maybe_t<int>{ 15 } }));
}
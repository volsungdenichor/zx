#include <gmock/gmock.h>

#include <zx/pipe.hpp>

TEST(pipe, create)
{
    constexpr auto pipe = zx::pipe(
        std::plus<>{}, [](int x) { return std::to_string(x); }, [](const std::string& s) { return "[" + s + "]"; });

    EXPECT_THAT(pipe(3, 4), testing::Eq("[7]"));
    EXPECT_THAT(pipe(2, 10), testing::Eq("[12]"));
}

TEST(pipe, appending_pipe_to_existing_pipe)
{
    constexpr auto pipe = zx::pipe(
        std::plus<>{}, [](int x) { return std::to_string(x); }, [](const std::string& s) { return "[" + s + "]"; });
    constexpr auto extended_pipe = zx::pipe(pipe, [](const std::string& s) { return s + "..."; });

    EXPECT_THAT(pipe(3, 4), testing::Eq("[7]"));
    EXPECT_THAT(pipe(2, 10), testing::Eq("[12]"));
    EXPECT_THAT(extended_pipe(3, 4), testing::Eq("[7]..."));
    EXPECT_THAT(extended_pipe(2, 10), testing::Eq("[12]..."));
}

TEST(pipe, pipeline_operator)
{
    const auto pipe = zx::fn(std::plus<>{})                 //
        |= zx::fn([](int x) { return std::to_string(x); })  //
        |= zx::fn([](const std::string& s) { return "[" + s + "]"; });

    EXPECT_THAT(pipe(3, 4), testing::Eq("[7]"));
    EXPECT_THAT(pipe(2, 10), testing::Eq("[12]"));
}

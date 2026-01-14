#include <gmock/gmock.h>

#include <zx/sequence.hpp>

TEST(sequence, default_constructed_is_empty)
{
    zx::sequence<int> seq = zx::sequence<int>{};

    const std::vector<int> result = seq;

    EXPECT_THAT(result, testing::IsEmpty());
}

TEST(sequence, from_iterator)
{
    std::vector<int> vec = { 1, 2, 3, 4, 5 };
    zx::sequence<int> seq = zx::sequence<int>(vec.begin(), vec.end());

    const std::vector<int> result = seq;

    EXPECT_THAT(result, testing::ElementsAre(1, 2, 3, 4, 5));
}

TEST(sequence, from_range)
{
    std::vector<int> vec = { 1, 2, 3, 4, 5 };
    zx::sequence<int> seq = zx::sequence<int>(vec);

    const std::vector<int> result = seq;

    EXPECT_THAT(result, testing::ElementsAre(1, 2, 3, 4, 5));
}

TEST(sequence, from_owning_range)
{
    std::vector<int> vec = { 1, 2, 3, 4, 5 };
    zx::sequence<int> seq = zx::sequence<int>(std::move(vec), 0);

    const std::vector<int> result = seq;

    EXPECT_THAT(result, testing::ElementsAre(1, 2, 3, 4, 5));
}

TEST(sequence, inspect)
{
    std::vector<int> vec = { 1, 2, 3 };
    zx::sequence<int> seq = zx::sequence<int>(vec);

    std::vector<int> inspected;

    const std::vector<int> result = seq.inspect([&inspected](int value) { inspected.push_back(value * 10); });

    EXPECT_THAT(result, testing::ElementsAre(1, 2, 3));
    EXPECT_THAT(inspected, testing::ElementsAre(10, 20, 30));
}

TEST(sequence, inspect_indexed)
{
    std::vector<int> vec = { 1, 2, 3 };
    zx::sequence<int> seq = zx::sequence<int>(vec);

    std::vector<std::pair<std::ptrdiff_t, int>> inspected;

    const std::vector<int> result
        = seq.inspect_indexed([&inspected](std::ptrdiff_t index, int value) { inspected.emplace_back(index, value * 10); });

    EXPECT_THAT(result, testing::ElementsAre(1, 2, 3));
    EXPECT_THAT(inspected, testing::ElementsAre(testing::Pair(0, 10), testing::Pair(1, 20), testing::Pair(2, 30)));
}

TEST(sequence, transform)
{
    std::vector<int> vec = { 1, 2, 3 };
    zx::sequence<int> seq = zx::sequence<int>(vec);

    const std::vector<int> result = seq.transform([](int value) { return value * 2; });

    EXPECT_THAT(result, testing::ElementsAre(2, 4, 6));
}

TEST(sequence, transform_indexed)
{
    std::vector<int> vec = { 1, 2, 3 };
    zx::sequence<int> seq = zx::sequence<int>(vec);

    const std::vector<int> result
        = seq.transform_indexed([](std::ptrdiff_t index, int value) { return value + static_cast<int>(index); });

    EXPECT_THAT(result, testing::ElementsAre(1, 3, 5));
}

TEST(sequence, filter)
{
    std::vector<int> vec = { 1, 2, 3, 4, 5, 6 };
    zx::sequence<int> seq = zx::sequence<int>(vec);

    const std::vector<int> result = seq.filter([](int value) { return value % 2 == 0; });

    EXPECT_THAT(result, testing::ElementsAre(2, 4, 6));
}

TEST(sequence, filter_indexed)
{
    std::vector<int> vec = { 10, 20, 30, 40, 50 };
    zx::sequence<int> seq = zx::sequence<int>(vec);

    const std::vector<int> result = seq.filter_indexed([](std::ptrdiff_t index, int) { return index % 2 == 0; });

    EXPECT_THAT(result, testing::ElementsAre(10, 30, 50));
}

TEST(sequence, transform_maybe)
{
    std::vector<int> vec = { 1, 2, 3, 4, 5 };
    zx::sequence<int> seq = zx::sequence<int>(vec);

    const std::vector<int> result = seq.transform_maybe(
        [](int value) -> zx::maybe<int>
        {
            if (value % 2 == 0)
            {
                return value * 10;
            }
            return zx::none;
        });

    EXPECT_THAT(result, testing::ElementsAre(20, 40));
}

TEST(sequence, transform_maybe_indexed)
{
    std::vector<int> vec = { 5, 10, 15, 20, 25 };
    zx::sequence<int> seq = zx::sequence<int>(vec);

    const std::vector<int> result = seq.transform_maybe_indexed(
        [](std::ptrdiff_t index, int value) -> zx::maybe<int>
        {
            if (index % 2 == 1)
            {
                return value + static_cast<int>(index);
            }
            return zx::none;
        });

    EXPECT_THAT(result, testing::ElementsAre(11, 23));
}

TEST(sequence, take)
{
    std::vector<int> vec = { 1, 2, 3, 4, 5 };
    zx::sequence<int> seq = zx::sequence<int>(vec);

    const std::vector<int> result = seq.take(3);

    EXPECT_THAT(result, testing::ElementsAre(1, 2, 3));
}

TEST(sequence, drop)
{
    std::vector<int> vec = { 1, 2, 3, 4, 5 };
    zx::sequence<int> seq = zx::sequence<int>(vec);

    const std::vector<int> result = seq.drop(2);

    EXPECT_THAT(result, testing::ElementsAre(3, 4, 5));
}

TEST(sequence, step)
{
    std::vector<int> vec = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    zx::sequence<int> seq = zx::sequence<int>(vec);

    const std::vector<int> result = seq.step(3);

    EXPECT_THAT(result, testing::ElementsAre(0, 3, 6, 9));
}

TEST(sequence, take_while)
{
    std::vector<int> vec = { 2, 4, 6, 7, 8 };
    zx::sequence<int> seq = zx::sequence<int>(vec);

    const std::vector<int> result = seq.take_while([](int value) { return value % 2 == 0; });

    EXPECT_THAT(result, testing::ElementsAre(2, 4, 6));
}

TEST(sequence, drop_while)
{
    std::vector<int> vec = { 1, 3, 5, 6, 7 };
    zx::sequence<int> seq = zx::sequence<int>(vec);

    const std::vector<int> result = seq.drop_while([](int value) { return value % 2 != 0; });

    EXPECT_THAT(result, testing::ElementsAre(6, 7));
}

TEST(sequence, take_while_indexed)
{
    std::vector<int> vec = { 10, 20, 30, 25, 40 };
    zx::sequence<int> seq = zx::sequence<int>(vec);

    const std::vector<int> result
        = seq.take_while_indexed([](std::ptrdiff_t index, int value) { return value > static_cast<int>(index * 10); });

    EXPECT_THAT(result, testing::ElementsAre(10, 20, 30));
}

TEST(sequence, drop_while_indexed)
{
    std::vector<int> vec = { 5, 15, 25, 20, 30 };
    zx::sequence<int> seq = zx::sequence<int>(vec);

    const std::vector<int> result
        = seq.drop_while_indexed([](std::ptrdiff_t index, int value) { return value > static_cast<int>(index * 10); });

    EXPECT_THAT(result, testing::ElementsAre(20, 30));
}

TEST(sequence, intersperse)
{
    std::vector<int> vec = { 1, 2, 3 };
    zx::sequence<int> seq = zx::sequence<int>(vec);

    const std::vector<int> result = seq.intersperse(0);

    EXPECT_THAT(result, testing::ElementsAre(1, 0, 2, 0, 3));
}

TEST(sequence, concat)
{
    zx::sequence<int> seq1 = zx::sequence<int>{ std::vector<int>{ 1, 2, 3 }, 0 };
    zx::sequence<int> seq2 = zx::sequence<int>{ std::vector<int>{ 4, 5 }, 0 };
    zx::sequence<int> seq3 = zx::sequence<int>{ std::vector<int>{ 6, 7, 8, 9 }, 0 };

    const std::vector<int> result = zx::seq::concat(seq1, seq2, seq3);

    EXPECT_THAT(result, testing::ElementsAre(1, 2, 3, 4, 5, 6, 7, 8, 9));
}

TEST(sequence, zip)
{
    zx::sequence<int> seq1 = zx::sequence<int>{ std::vector<int>{ 1, 2, 3 }, 0 };
    zx::sequence<char> seq2 = zx::sequence<char>{ std::vector<char>{ 'a', 'b', 'c', 'd' }, 0 };

    const std::vector<std::tuple<int, char>> result = zx::seq::zip(seq1, seq2);

    EXPECT_THAT(
        result, testing::ElementsAre(testing::FieldsAre(1, 'a'), testing::FieldsAre(2, 'b'), testing::FieldsAre(3, 'c')));
}

TEST(sequence, init)
{
    const std::vector<int> result = zx::seq::init(5, [](std::ptrdiff_t index) { return static_cast<int>(index * index); });

    EXPECT_THAT(result, testing::ElementsAre(0, 1, 4, 9, 16));
}

TEST(sequence, init_infinite)
{
    zx::sequence<int> seq = zx::seq::init_infinite([](std::ptrdiff_t index) { return static_cast<int>(index + 1); });

    const std::vector<int> result = seq.take(10);

    EXPECT_THAT(result, testing::ElementsAre(1, 2, 3, 4, 5, 6, 7, 8, 9, 10));
}

TEST(sequence, iota)
{
    zx::sequence<int> seq = zx::seq::iota(5);

    const std::vector<int> result = seq.take(10);

    EXPECT_THAT(result, testing::ElementsAre(5, 6, 7, 8, 9, 10, 11, 12, 13, 14));
}

TEST(sequence, range)
{
    zx::sequence<int> seq = zx::seq::range(3, 8);

    const std::vector<int> result = seq;

    EXPECT_THAT(result, testing::ElementsAre(3, 4, 5, 6, 7));
}

TEST(sequence, range_empty)
{
    zx::sequence<int> seq = zx::seq::range(10, 5);

    const std::vector<int> result = seq;

    EXPECT_THAT(result, testing::IsEmpty());
}

TEST(sequence, unfold)
{
    zx::sequence<std::string> seq = zx::seq::unfold(
        std::tuple<int, int, int>{ 0, 1, 1 },
        [](std::tuple<int, int, int> state) -> zx::maybe<std::pair<std::string, std::tuple<int, int, int>>>
        {
            const auto [n, a, b] = state;
            if (n < 10)
            {
                return { { std::to_string(a), { n + 1, b, a + b } } };
            }
            return zx::none;
        });

    const std::vector<std::string> result = seq;

    EXPECT_THAT(result, testing::ElementsAre("1", "1", "2", "3", "5", "8", "13", "21", "34", "55"));
}

TEST(sequence, repeat)
{
    zx::sequence<char> seq = zx::seq::repeat('x');

    const std::vector<char> result = seq.take(6);

    EXPECT_THAT(result, testing::ElementsAre('x', 'x', 'x', 'x', 'x', 'x'));
}

TEST(sequence, singleton)
{
    zx::sequence<int> seq = zx::seq::single(42);

    const std::vector<int> result = seq;

    EXPECT_THAT(result, testing::ElementsAre(42));
}

TEST(sequence, vec)
{
    zx::sequence<const int&> seq = zx::seq::vec(1, 2, 3, 4, 5);

    const std::vector<int> result = seq;

    EXPECT_THAT(result, testing::ElementsAre(1, 2, 3, 4, 5));
}

TEST(sequence, join)
{
    zx::sequence<zx::sequence<const int&>> seq_of_seqs = zx::seq::vec(zx::seq::vec(1, 2), zx::seq::vec(3), zx::seq::vec(4, 5, 6));

    const std::vector<int> result = seq_of_seqs.join();

    EXPECT_THAT(result, testing::ElementsAre(1, 2, 3, 4, 5, 6));
}
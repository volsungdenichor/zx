#include <gmock/gmock.h>

#include <zx/iterator_range.hpp>

TEST(iteratror_range, slicing)
{
    const std::vector<int> vect = { 1, 2, 3, 4, 5, 6, 7 };
    const auto span = zx::span_t<int>{ vect };
    EXPECT_THAT(span, testing::ElementsAre(1, 2, 3, 4, 5, 6, 7));

    EXPECT_THAT(span.take(4), testing::ElementsAre(1, 2, 3, 4));
    EXPECT_THAT(span.drop(4), testing::ElementsAre(5, 6, 7));
    EXPECT_THAT(span.take_back(4), testing::ElementsAre(4, 5, 6, 7));
    EXPECT_THAT(span.drop_back(4), testing::ElementsAre(1, 2, 3));

    EXPECT_THAT(span.slice({ {}, 3 }), testing::ElementsAre(1, 2, 3));
    EXPECT_THAT(span.slice({ 0, 3 }), testing::ElementsAre(1, 2, 3));
    EXPECT_THAT(span.slice({ 1, 3 }), testing::ElementsAre(2, 3));
    EXPECT_THAT(span.slice({ 3, {} }), testing::ElementsAre(4, 5, 6, 7));
    EXPECT_THAT(span.slice({ 3, 5 }), testing::ElementsAre(4, 5));
    EXPECT_THAT(span.slice({ -2, {} }), testing::ElementsAre(6, 7));
    EXPECT_THAT(span.slice({ {}, -2 }), testing::ElementsAre(1, 2, 3, 4, 5));
    EXPECT_THAT(span.slice({ -4, -2 }), testing::ElementsAre(4, 5));
    EXPECT_THAT(span.slice({ -2, -4 }), testing::IsEmpty());
}

#include <gmock/gmock.h>

#include <zx/nested_text.hpp>

namespace
{
struct TestStruct
{
    int a;
    std::string b;
    std::vector<int> c;
};

}  // namespace

template <>
struct zx::nested_text::codec_t<TestStruct> : public zx::nested_text::struct_codec_t<TestStruct>
{
    using _ = TestStruct;
#define MEMBER(name) member_t{ #name, &_::name }
    codec_t() : struct_codec_t{ { MEMBER(a), MEMBER(b), MEMBER(c) } } { }
#undef MEMBER
};

TEST(nested_text_serialization, integer)
{
    EXPECT_THAT(zx::nested_text::encode(123), testing::Eq(zx::nested_text::value_t{ "123" }));
    EXPECT_THAT((zx::nested_text::decode<int>(zx::nested_text::value_t{ "123" })), testing::Eq(123));
}

TEST(nested_text_serialization, integer_rejects_trailing_garbage)
{
    EXPECT_THROW((void)zx::nested_text::decode<int>(zx::nested_text::value_t{ "123abc" }), std::runtime_error);
}

TEST(nested_text_serialization, string)
{
    EXPECT_THAT(zx::nested_text::encode(std::string{ "Hello" }), testing::Eq(zx::nested_text::value_t{ "Hello" }));
    EXPECT_THAT((zx::nested_text::decode<std::string>(zx::nested_text::value_t{ "Hello" })), testing::Eq("Hello"));
}

TEST(nested_text_serialization, vector)
{
    EXPECT_THAT(
        zx::nested_text::encode(std::vector<int>{ 1, 2, 3 }),
        testing::Eq(zx::nested_text::value_t{ zx::nested_text::list_t{ "1", "2", "3" } }));
    EXPECT_THAT(
        (zx::nested_text::decode<std::vector<int>>(zx::nested_text::value_t{ zx::nested_text::list_t{ "1", "2", "3" } })),
        testing::Eq(std::vector<int>{ 1, 2, 3 }));
}

TEST(nested_text_serialization, struct)
{
    TestStruct test_struct{ 42, "Answer", { 1, 2, 3 } };
    EXPECT_THAT(
        zx::nested_text::encode(test_struct),
        testing::Eq(zx::nested_text::value_t{ zx::nested_text::map_t{
            { "a", "42" }, { "b", "Answer" }, { "c", zx::nested_text::list_t{ "1", "2", "3" } } } }));
    EXPECT_THAT(
        (zx::nested_text::decode<TestStruct>(zx::nested_text::value_t{ zx::nested_text::map_t{
            { "a", "42" }, { "b", "Answer" }, { "c", zx::nested_text::list_t{ "1", "2", "3" } } } })),
        testing::AllOf(
            testing::Field("a", &TestStruct::a, testing::Eq(42)),
            testing::Field("b", &TestStruct::b, testing::Eq("Answer")),
            testing::Field("c", &TestStruct::c, testing::ElementsAre(1, 2, 3))));
}
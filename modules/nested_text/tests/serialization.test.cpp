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
    EXPECT_THAT(zx::nested_text::encode(123), testing::Eq(zx::nested_text::node_t{ "123" }));
    EXPECT_THAT((zx::nested_text::decode<int>(zx::nested_text::node_t{ "123" })), testing::Eq(123));
}

TEST(nested_text_serialization, integer_rejects_trailing_garbage)
{
    EXPECT_THAT(
        [&]() { zx::nested_text::decode<int>(zx::nested_text::node_t{ "123abc" }); },
        testing::ThrowsMessage<std::runtime_error>(testing::HasSubstr("Failed to decode int")));
}

TEST(nested_text_serialization, string)
{
    EXPECT_THAT(zx::nested_text::encode(std::string{ "Hello" }), testing::Eq(zx::nested_text::node_t{ "Hello" }));
    EXPECT_THAT((zx::nested_text::decode<std::string>(zx::nested_text::node_t{ "Hello" })), testing::Eq("Hello"));
}

TEST(nested_text_serialization, vector)
{
    EXPECT_THAT(
        zx::nested_text::encode(std::vector<int>{ 1, 2, 3 }),
        testing::Eq(zx::nested_text::node_t{ zx::nested_text::list_t{ "1", "2", "3" } }));
    EXPECT_THAT(
        (zx::nested_text::decode<std::vector<int>>(zx::nested_text::node_t{ zx::nested_text::list_t{ "1", "2", "3" } })),
        testing::Eq(std::vector<int>{ 1, 2, 3 }));
}

TEST(nested_text_serialization, bool)
{
    EXPECT_THAT(zx::nested_text::encode(true), testing::Eq(zx::nested_text::node_t{ "true" }));
    EXPECT_THAT(zx::nested_text::encode(false), testing::Eq(zx::nested_text::node_t{ "false" }));
    EXPECT_THAT((zx::nested_text::decode<bool>(zx::nested_text::node_t{ "true" })), testing::Eq(true));
    EXPECT_THAT((zx::nested_text::decode<bool>(zx::nested_text::node_t{ "false" })), testing::Eq(false));
    EXPECT_THAT(
        []() { zx::nested_text::decode<bool>(zx::nested_text::list_t{ "not a bool" }); },
        testing::ThrowsMessage<std::runtime_error>(
            testing::HasSubstr("Failed to decode bool: expected type: string, actual: list")));
}

TEST(nested_text_serialization, optional)
{
    EXPECT_THAT(
        zx::nested_text::encode(std::optional<int>{}), testing::Eq(zx::nested_text::node_t{ zx::nested_text::map_t{} }));
    EXPECT_THAT(
        zx::nested_text::encode(std::optional<int>{ 42 }),
        testing::Eq(zx::nested_text::node_t{ zx::nested_text::map_t{ { "some", "42" } } }));
    EXPECT_THAT(
        (zx::nested_text::decode<std::optional<int>>(zx::nested_text::node_t{ zx::nested_text::map_t{} })),
        testing::Eq(std::nullopt));
    EXPECT_THAT(
        (zx::nested_text::decode<std::optional<int>>(zx::nested_text::node_t{ zx::nested_text::map_t{ { "some", "42" } } })),
        testing::Eq(std::optional{ 42 }));
}

TEST(nested_text_serialization, maybe)
{
    EXPECT_THAT(
        zx::nested_text::encode(zx::maybe_t<int>{}), testing::Eq(zx::nested_text::node_t{ zx::nested_text::map_t{} }));
    EXPECT_THAT(
        zx::nested_text::encode(zx::maybe_t<int>{ 42 }),
        testing::Eq(zx::nested_text::node_t{ zx::nested_text::map_t{ { "some", "42" } } }));
    EXPECT_THAT(
        (zx::nested_text::decode<zx::maybe_t<int>>(zx::nested_text::node_t{ zx::nested_text::map_t{} })),
        testing::Eq(zx::maybe_t<int>{}));
    EXPECT_THAT(
        (zx::nested_text::decode<zx::maybe_t<int>>(zx::nested_text::node_t{ zx::nested_text::map_t{ { "some", "42" } } })),
        testing::Eq(zx::maybe_t<int>{ 42 }));
}

TEST(nested_text_serialization, result)
{
    EXPECT_THAT(
        zx::nested_text::encode(zx::result_t<int, std::string>{ 42 }),
        testing::Eq(zx::nested_text::node_t{ zx::nested_text::map_t{ { "ok", "42" } } }));
    EXPECT_THAT(
        zx::nested_text::encode(zx::result_t<int, std::string>{ zx::error("error message") }),
        testing::Eq(zx::nested_text::node_t{ zx::nested_text::map_t{ { "error", "error message" } } }));
    EXPECT_THAT(
        (zx::nested_text::decode<zx::result_t<int, std::string>>(
            zx::nested_text::node_t{ zx::nested_text::map_t{ { "ok", "42" } } })),
        testing::Eq(zx::result_t<int, std::string>{ 42 }));
    EXPECT_THAT(
        (zx::nested_text::decode<zx::result_t<int, std::string>>(
            zx::nested_text::node_t{ zx::nested_text::map_t{ { "error", "error message" } } })),
        testing::Eq(zx::result_t<int, std::string>{ zx::error("error message") }));
}

TEST(nested_text_serialization, struct)
{
    TestStruct test_struct{ 42, "Answer", { 1, 2, 3 } };
    EXPECT_THAT(
        zx::nested_text::encode(test_struct),
        testing::Eq(zx::nested_text::node_t{ zx::nested_text::map_t{
            { "a", "42" }, { "b", "Answer" }, { "c", zx::nested_text::list_t{ "1", "2", "3" } } } }));
    EXPECT_THAT(
        (zx::nested_text::decode<TestStruct>(zx::nested_text::node_t{ zx::nested_text::map_t{
            { "a", "42" }, { "b", "Answer" }, { "c", zx::nested_text::list_t{ "1", "2", "3" } } } })),
        testing::AllOf(
            testing::Field("a", &TestStruct::a, testing::Eq(42)),
            testing::Field("b", &TestStruct::b, testing::Eq("Answer")),
            testing::Field("c", &TestStruct::c, testing::ElementsAre(1, 2, 3))));
}

TEST(nested_text_serialization, result_of_parsing_of_the_serialized_struct_is_equal_to_the_original_struct)
{
    TestStruct test_struct{ 42, "Answer", { 1, 2, 3 } };
    EXPECT_THAT(
        (zx::nested_text::decode<TestStruct>(zx::nested_text::encode(test_struct))),
        testing::AllOf(
            testing::Field("a", &TestStruct::a, testing::Eq(42)),
            testing::Field("b", &TestStruct::b, testing::Eq("Answer")),
            testing::Field("c", &TestStruct::c, testing::ElementsAre(1, 2, 3))));
}

TEST(nested_text_serialization, result_of_text_parsing_of_the_serialized_struct_is_equal_to_the_original_struct)
{
    TestStruct test_struct{ 42, "Answer", { 1, 2, 3 } };
    EXPECT_THAT(
        (zx::nested_text::decode<TestStruct>(zx::nested_text::parse(zx::str(zx::nested_text::encode(test_struct))))),
        testing::AllOf(
            testing::Field("a", &TestStruct::a, testing::Eq(42)),
            testing::Field("b", &TestStruct::b, testing::Eq("Answer")),
            testing::Field("c", &TestStruct::c, testing::ElementsAre(1, 2, 3))));
}

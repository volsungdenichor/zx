#include <gmock/gmock.h>

#include <zx/predicates.hpp>

namespace
{

constexpr auto is_success = [](auto&& matcher)
{
    return testing::ResultOf(
        "is_success",
        [](const zx::predicates::validation_result_t& result) { return result.is_success(); },
        std::forward<decltype(matcher)>(matcher));
};

struct Date
{
    int year;
    int month;
    int day;
};

struct Person
{
    std::string name;
    Date date_of_birth;
};

}  // namespace

template <>
struct zx::nested_text::codec_t<Date> : public zx::nested_text::struct_codec_t<Date>
{
    codec_t() : struct_codec_t{ { { "year", &Date::year }, { "month", &Date::month }, { "day", &Date::day } } } { }
};

template <>
struct zx::nested_text::codec_t<Person> : public zx::nested_text::struct_codec_t<Person>
{
    codec_t() : struct_codec_t{ { { "name", &Person::name }, { "date_of_birth", &Person::date_of_birth } } } { }
};

TEST(predicates, each_element_greater_than)
{
    const auto pred = zx::predicates::each_element(zx::predicates::result_of(
        "date_of_birth",
        &Person::date_of_birth,
        zx::predicates::result_of(
            "year", &Date::year, zx::predicates::all(zx::predicates::gt(1990), zx::predicates::lt(2000)))));
    EXPECT_THAT(
        pred.format(),
        (zx::nested_text::list_t{
            "each_element",
            zx::nested_text::list_t{
                "result_of",
                "date_of_birth",
                zx::nested_text::list_t{ "result_of", "year", zx::nested_text::list_t{ "gt", "1990" } } } }));
    EXPECT_THAT(
        pred(std::vector{
            Person{ "Alice", { 1901, 5, 12 } }, Person{ "Bob", { 1995, 7, 23 } }, Person{ "Charlie", { 2000, 8, 15 } } }),
        testing::IsTrue());
    EXPECT_THAT(
        pred.validate(std::vector{
            Person{ "Alice", { 1901, 5, 12 } }, Person{ "Bob", { 1995, 7, 23 } }, Person{ "Charlie", { 2000, 8, 15 } } }),
        is_success(true));

    EXPECT_THAT(
        pred(std::vector{
            Person{ "Alice", { 1901, 5, 12 } }, Person{ "Bob", { 1995, 7, 23 } }, Person{ "Charlie", { 2000, 8, 15 } } }),
        testing::IsFalse());
    EXPECT_THAT(
        pred.validate(std::vector{
            Person{ "Alice", { 1901, 5, 12 } }, Person{ "Bob", { 1995, 7, 23 } }, Person{ "Charlie", { 2000, 8, 15 } } }),
        is_success(true));
}

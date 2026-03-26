#include <gmock/gmock.h>

#include <sstream>
#include <string>
#include <vector>
#include <zx/predicates.hpp>

namespace
{

constexpr auto when_split = [](auto&& matcher)
{
    return testing::ResultOf(
        "split",
        [](const auto& value)
        {
            std::istringstream is(value);
            std::vector<std::string> lines;
            std::string line;
            while (std::getline(is, line))
            {
                if (!line.empty() && line.back() == '\r')
                {
                    line.pop_back();
                }
                lines.push_back(line);
            }
            return lines;
        },
        std::forward<decltype(matcher)>(matcher));
};

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
                ":date_of_birth",
                zx::nested_text::list_t{
                    ":year",
                    zx::nested_text::list_t{ "all",
                                             zx::nested_text::list_t{ zx::nested_text::list_t{ "gt", "1990" },
                                                                      zx::nested_text::list_t{ "lt", "2000" } } } } } }));
    EXPECT_THAT(
        pred(std::vector{
            Person{ "Alice", { 1991, 5, 12 } }, Person{ "Bob", { 1995, 7, 23 } }, Person{ "Charlie", { 1999, 8, 15 } } }),
        testing::IsTrue());
    EXPECT_THAT(
        pred.validate(std::vector{
            Person{ "Alice", { 1991, 5, 12 } }, Person{ "Bob", { 1995, 7, 23 } }, Person{ "Charlie", { 1999, 8, 15 } } }),
        is_success(true));

    EXPECT_THAT(
        pred(std::vector{
            Person{ "Alice", { 1901, 5, 12 } }, Person{ "Bob", { 1995, 7, 23 } }, Person{ "Charlie", { 2000, 8, 15 } } }),
        testing::IsFalse());
    EXPECT_THAT(
        pred.validate(std::vector{
            Person{ "Alice", { 1901, 5, 12 } }, Person{ "Bob", { 1995, 7, 23 } }, Person{ "Charlie", { 2000, 8, 15 } } }),
        is_success(false));
}

TEST(predicates, validate_failure_output_is_concise)
{
    const auto pred = zx::predicates::each_element(zx::predicates::result_of(
        "date_of_birth",
        &Person::date_of_birth,
        zx::predicates::result_of(
            "year", &Date::year, zx::predicates::all(zx::predicates::gt(1990), zx::predicates::lt(2000)))));

    std::ostringstream os;
    os << pred.validate(std::vector{
        Person{ "Alice", { 1901, 5, 12 } }, Person{ "Bob", { 1995, 7, 23 } }, Person{ "Charlie", { 2000, 8, 15 } } });

    const std::vector<std::string> expected_lines = {
        "failure:",
        "```",
        "{",
        "  :actual {:type \"std::vector<(anonymous namespace)::Person, std::allocator<(anonymous namespace)::Person> >\"}",
        "  :failing_predicate [",
        "    each_element",
        "    [",
        "      :date_of_birth",
        "      [",
        "        :year",
        "        [",
        "          all",
        "          [",
        "            [gt 1990]",
        "            [lt 2000]",
        "          ]",
        "        ]",
        "      ]",
        "    ]",
        "  ]",
        "  :failure_message {",
        "    :message \"some elements did not satisfy the predicate\"",
        "    :count {:total 3 :failed 2}",
        "    :failed_elements [",
        "      {",
        "        :index 0",
        "        :failure_message {",
        "          :member date_of_birth",
        "          :failure_message {",
        "            :member year",
        "            :failure_message {",
        "              :message \"some predicates did not satisfy the condition\"",
        "              :count {:total 2 :failed 1}",
        "              :failed_predicates [",
        "                {",
        "                  :predicate [gt 1990]",
        "                  :failure_message [expected [gt 1990]]",
        "                }",
        "              ]",
        "            }",
        "          }",
        "        }",
        "      }",
        "      {",
        "        :index 2",
        "        :failure_message {",
        "          :member date_of_birth",
        "          :failure_message {",
        "            :member year",
        "            :failure_message {",
        "              :message \"some predicates did not satisfy the condition\"",
        "              :count {:total 2 :failed 1}",
        "              :failed_predicates [",
        "                {",
        "                  :predicate [lt 2000]",
        "                  :failure_message [expected [lt 2000]]",
        "                }",
        "              ]",
        "            }",
        "          }",
        "        }",
        "      }",
        "    ]",
        "  }",
        "}",
        "```",
    };

    EXPECT_THAT(os.str(), when_split(testing::ElementsAreArray(expected_lines)));
}

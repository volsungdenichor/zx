#include <gmock/gmock.h>

#include <zx/nested_text.hpp>

namespace
{
constexpr auto WhenSerialized = [](auto&& matcher)
{
    return testing::ResultOf(
        "serialized",
        [](const auto& value)
        {
            std::ostringstream os;
            os << value;
            return os.str();
        },
        std::forward<decltype(matcher)>(matcher));
};

constexpr auto WhenLineSplit = [](auto&& matcher)
{
    return testing::ResultOf(
        "line-split",
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

void ExpectParseError(std::string_view text, std::string_view message)
{
    try
    {
        static_cast<void>(zx::nested_text::parse(text));
        FAIL() << "Expected parse_error";
    }
    catch (const zx::nested_text::parse_error& error)
    {
        EXPECT_THAT(error.what(), testing::HasSubstr(message));
    }
}
}  // namespace

TEST(nested_text, empty)
{
    EXPECT_THAT(zx::nested_text::value_t{}, WhenSerialized(R"("")"));
}

TEST(nested_text, initial)
{
    EXPECT_THAT(zx::nested_text::value_t{ "123" }, WhenSerialized("123"));
}

TEST(nested_text, quoted)
{
    EXPECT_THAT(zx::nested_text::value_t{ "Z Y" }, WhenSerialized(R"("Z Y")"));
}

TEST(nested_text, list)
{
    EXPECT_THAT(
        (zx::nested_text::value_t{ zx::nested_text::list_t{ "123", zx::nested_text::list_t{ "X", "Y y" } } }),
        WhenSerialized(R"([123 [X "Y y"]])"));
}

TEST(nested_text, map)
{
    EXPECT_THAT(
        (zx::nested_text::value_t{
            zx::nested_text::map_t{ { "a", "123" }, { "b", zx::nested_text::list_t{ "X", "Y y" } } } }),
        WhenSerialized(R"({:a 123 :b [X "Y y"]})"));
}

TEST(nested_text, tree)
{
    EXPECT_THAT(
        (zx::nested_text::value_t{ zx::nested_text::list_t{
            "World",
            zx::nested_text::list_t{
                "Europe",
                zx::nested_text::list_t{
                    "Germany",
                    "Poland",
                    "France",
                },
            },
            zx::nested_text::list_t{
                "Asia",
                zx::nested_text::list_t{
                    "China",
                    "Japan",
                    "India",
                },
            },
        } }),
        WhenSerialized(R"([World [Europe [Germany Poland France]] [Asia [China Japan India]]])"));
}

TEST(nested_text, parse_empty)
{
    EXPECT_THAT(zx::nested_text::parse(""), testing::Eq(zx::nested_text::value_t{}));
}

TEST(nested_text, parse_string_with_escapes)
{
    EXPECT_THAT(zx::nested_text::parse("\"line\\n\\t\\\"\\\\\""), testing::Eq(zx::nested_text::value_t{ "line\n\t\"\\" }));
}

TEST(nested_text, parse_list_with_comments_and_commas)
{
    EXPECT_THAT(
        zx::nested_text::parse("[alpha, ; ignore this\n \"beta gamma\"]"),
        testing::Eq(zx::nested_text::value_t{ zx::nested_text::list_t{ "alpha", "beta gamma" } }));
}

TEST(nested_text, parse_map_with_nested_values)
{
    EXPECT_THAT(
        zx::nested_text::parse(R"({:a 123 :b [X "Y y"]})"),
        testing::Eq(zx::nested_text::value_t{ zx::nested_text::map_t{
            { "a", "123" },
            { "b", zx::nested_text::list_t{ "X", "Y y" } },
        } }));
}

TEST(nested_text, parse_multiple_top_level_values_as_list)
{
    EXPECT_THAT(
        zx::nested_text::parse(R"(123 "two words" x)"),
        testing::Eq(zx::nested_text::value_t{ zx::nested_text::list_t{
            "123",
            "two words",
            "x",
        } }));
}

TEST(nested_text, parse_invalid_escape_reports_error)
{
    ExpectParseError(R"("bad\q")", "Invalid escape sequence: \\q");
}

TEST(nested_text, parse_map_without_value_reports_error)
{
    ExpectParseError("{:a}", "Map requires an even number of elements");
}

TEST(nested_text, parse_top_level_colon_reports_error)
{
    ExpectParseError(":", "Unexpected delimiter: :");
}

TEST(nested_text, mixed_type_ordering)
{
    const zx::nested_text::value_t as_string{ "s" };
    const zx::nested_text::value_t as_list{ zx::nested_text::list_t{ "s" } };
    const zx::nested_text::value_t as_map{ zx::nested_text::map_t{ { "k", "v" } } };

    EXPECT_TRUE(as_string < as_list);
    EXPECT_TRUE(as_list < as_map);
    EXPECT_TRUE(as_string < as_map);

    EXPECT_FALSE(as_list < as_string);
    EXPECT_FALSE(as_map < as_list);
    EXPECT_FALSE(as_map < as_string);
}

TEST(nested_text, tree_pretty_print)
{
    EXPECT_THAT(
        (zx::nested_text::to_pretty_string(zx::nested_text::value_t{ zx::nested_text::list_t{
            "World",
            zx::nested_text::list_t{
                "Europe",
                zx::nested_text::list_t{
                    "Germany",
                    "Poland",
                    "France",
                },
            },
            zx::nested_text::list_t{
                "Asia",
                zx::nested_text::list_t{
                    "China",
                    "Japan",
                    "India",
                },
            },
        } })),
        WhenLineSplit(testing::ElementsAre(
            "[",
            "  World",
            "  [",
            "    Europe",
            "    [Germany Poland France]",
            "  ]",
            "  [",
            "    Asia",
            "    [China Japan India]",
            "  ]",
            "]")));
}
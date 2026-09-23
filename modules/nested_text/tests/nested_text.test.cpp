#include <gmock/gmock.h>

#include <zx/nested_text.hpp>
#include <zx/test/functional_matcher.hpp>

namespace
{
std::vector<std::string> split_lines(const std::string& text)
{
    std::istringstream is(text);
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
}

constexpr auto WhenSerialized = [](testing::Matcher<const std::string&> matcher)
{
    return zx::test::FunctionalMatcher{ [](const zx::nested_text::node_t& value,
                                           const testing::Matcher<const std::string&>& m) -> std::optional<std::string>
                                        {
                                            if (auto res = zx::test::validate(zx::format(value), m); res.has_value())
                                            {
                                                return "\nwhich does not match";
                                            }
                                            return std::nullopt;
                                        },
                                        [](std::ostream& os, bool positive, const testing::Matcher<const std::string&>& m)
                                        { os << "result of serialization " << zx::test::format_matcher(m, positive); },
                                        std::move(matcher) };
};

constexpr auto WhenSplitByLines = [](testing::Matcher<const std::vector<std::string>&> matcher)
{
    return zx::test::FunctionalMatcher{
        [](const std::string& value,
           const testing::Matcher<const std::vector<std::string>&>& m) -> std::optional<std::string>
        {
            const std::vector<std::string> lines = split_lines(value);
            if (auto res = zx::test::validate(lines, m); res.has_value())
            {
                std::stringstream ss;
                ss << "\nwhose result of line-split is:";
                for (std::size_t i = 0; i < lines.size(); ++i)
                {
                    ss << "\n  [" << std::setw(2) << i << "] " << lines[i];
                }
                ss << "\nand " << res->message;
                return ss.str();
            }
            return std::nullopt;
        },
        [](std::ostream& os, bool positive, const testing::Matcher<const std::vector<std::string>&>& m)
        { os << "result of line-split " << zx::test::format_matcher(m, positive); },
        std::move(matcher)
    };
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
    EXPECT_THAT(zx::nested_text::node_t{}, WhenSerialized(R"("")"));
}

TEST(nested_text, initial)
{
    EXPECT_THAT(zx::nested_text::node_t{ "123" }, WhenSerialized("123"));
}

TEST(nested_text, quoted)
{
    EXPECT_THAT(zx::nested_text::node_t{ "Z Y" }, WhenSerialized(R"("Z Y")"));
}

TEST(nested_text, list)
{
    EXPECT_THAT(
        (zx::nested_text::node_t{ zx::nested_text::list_t{ "123", zx::nested_text::list_t{ "X", "Y y" } } }),
        WhenSerialized(R"([123 [X "Y y"]])"));
}

TEST(nested_text, map)
{
    EXPECT_THAT(
        (zx::nested_text::node_t{
            zx::nested_text::map_t{ { "a", "123" }, { "b", zx::nested_text::list_t{ "X", "Y y" } } } }),
        WhenSerialized(R"({:a 123 :b [X "Y y"]})"));
}

TEST(nested_text, tree)
{
    EXPECT_THAT(
        (zx::nested_text::node_t{ zx::nested_text::list_t{
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
    EXPECT_THAT(zx::nested_text::parse(""), testing::Eq(zx::nested_text::node_t{}));
}

TEST(nested_text, parse_string_with_escapes)
{
    EXPECT_THAT(zx::nested_text::parse("\"line\\n\\t\\\"\\\\\""), testing::Eq(zx::nested_text::node_t{ "line\n\t\"\\" }));
}

TEST(nested_text, parse_list_with_comments_and_commas)
{
    EXPECT_THAT(
        zx::nested_text::parse("[alpha, ; ignore this\n \"beta gamma\"]"),
        testing::Eq(zx::nested_text::node_t{ zx::nested_text::list_t{ "alpha", "beta gamma" } }));
}

TEST(nested_text, parse_map_with_nested_values)
{
    EXPECT_THAT(
        zx::nested_text::parse(R"({:a 123 :b [X "Y y"]})"),
        testing::Eq(zx::nested_text::node_t{ zx::nested_text::map_t{
            { "a", "123" },
            { "b", zx::nested_text::list_t{ "X", "Y y" } },
        } }));
}

TEST(nested_text, parse_multiple_top_level_values_as_list)
{
    EXPECT_THAT(
        zx::nested_text::parse(R"(123 "two words" x)"),
        testing::Eq(zx::nested_text::node_t{ zx::nested_text::list_t{
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
    const zx::nested_text::node_t as_string{ "s" };
    const zx::nested_text::node_t as_list{ zx::nested_text::list_t{ "s" } };
    const zx::nested_text::node_t as_map{ zx::nested_text::map_t{ { "k", "v" } } };

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
        (zx::nested_text::to_pretty_string(zx::nested_text::node_t{ zx::nested_text::list_t{
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
        WhenSplitByLines(testing::ElementsAre(
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

TEST(nested_text, path_based_access)
{
    zx::nested_text::node_t data = zx::nested_text::parse(R"(
        [
            { :name "Alice" :age 30 }
            { :name "Bob" :age 25 }
        ]
    )");

    EXPECT_THAT(data.get(zx::nested_text::list_t{ "0", "name" }), testing::Optional(zx::nested_text::string_t{ "Alice" }));
    EXPECT_THAT(data.get(zx::nested_text::list_t{ "0", "age" }), testing::Optional(zx::nested_text::string_t{ "30" }));
    EXPECT_THAT(data.get(zx::nested_text::list_t{ "1", "name" }), testing::Optional(zx::nested_text::string_t{ "Bob" }));
    EXPECT_THAT(data.get(zx::nested_text::list_t{ "1", "age" }), testing::Optional(zx::nested_text::string_t{ "25" }));
    EXPECT_THAT(
        data.get(zx::nested_text::parse(R"([1 age])").as_list()), testing::Optional(zx::nested_text::string_t{ "25" }));

    EXPECT_THAT(data.get(zx::nested_text::list_t{ "2", "name" }), testing::Eq(std::nullopt));
    EXPECT_THAT(data.get(zx::nested_text::list_t{ "0", "nonexistent" }), testing::Eq(std::nullopt));
}

#include <zx/zx.hpp>

#include "matchers.hpp"

using namespace std::string_view_literals;

TEST_CASE("predicates - format", "")
{
    REQUIRE_THAT(  //
        zx::str(zx::all(zx::ge(0), zx::lt(5))),
        matchers::equal_to("(all (ge 0) (lt 5))"sv));
    REQUIRE_THAT(  //
        zx::str(zx::any(1, 2, 3, zx::ge(100))),
        matchers::equal_to("(any 1 2 3 (ge 100))"sv));
    REQUIRE_THAT(  //
        zx::str(zx::negate(zx::any(1, 2, 3))),
        matchers::equal_to("(not (any 1 2 3))"sv));
    REQUIRE_THAT(  //
        zx::str(zx::each_item(zx::any(1, 2, 3))),
        matchers::equal_to("(each_item (any 1 2 3))"sv));
    REQUIRE_THAT(  //
        zx::str(zx::contains_item(zx::any(1, 2, 3))),
        matchers::equal_to("(contains_item (any 1 2 3))"sv));
    REQUIRE_THAT(  //
        zx::str(zx::size_is(zx::lt(8))),
        matchers::equal_to("(size_is (lt 8))"sv));
}

TEST_CASE("predicates - eq", "")
{
    const auto pred = zx::eq(10);
    REQUIRE_THAT(  //
        zx::str(pred),
        matchers::equal_to("(eq 10)"sv));
    REQUIRE_THAT(pred(10), matchers::equal_to(true));
    REQUIRE_THAT(pred(5), matchers::equal_to(false));
    REQUIRE_THAT(pred(15), matchers::equal_to(false));
}

TEST_CASE("predicates - ne", "")
{
    const auto pred = zx::ne(10);
    REQUIRE_THAT(  //
        zx::str(pred),
        matchers::equal_to("(ne 10)"sv));
    REQUIRE_THAT(pred(10), matchers::equal_to(false));
    REQUIRE_THAT(pred(5), matchers::equal_to(true));
    REQUIRE_THAT(pred(15), matchers::equal_to(true));
}

TEST_CASE("predicates - lt", "")
{
    const auto pred = zx::lt(10);
    REQUIRE_THAT(  //
        zx::str(pred),
        matchers::equal_to("(lt 10)"sv));
    REQUIRE_THAT(pred(10), matchers::equal_to(false));
    REQUIRE_THAT(pred(5), matchers::equal_to(true));
    REQUIRE_THAT(pred(15), matchers::equal_to(false));
}

TEST_CASE("predicates - gt", "")
{
    const auto pred = zx::gt(10);
    REQUIRE_THAT(  //
        zx::str(pred),
        matchers::equal_to("(gt 10)"sv));
    REQUIRE_THAT(pred(10), matchers::equal_to(false));
    REQUIRE_THAT(pred(5), matchers::equal_to(false));
    REQUIRE_THAT(pred(15), matchers::equal_to(true));
}

TEST_CASE("predicates - le", "")
{
    const auto pred = zx::le(10);
    REQUIRE_THAT(  //
        zx::str(pred),
        matchers::equal_to("(le 10)"sv));
    REQUIRE_THAT(pred(10), matchers::equal_to(true));
    REQUIRE_THAT(pred(5), matchers::equal_to(true));
    REQUIRE_THAT(pred(15), matchers::equal_to(false));
}

TEST_CASE("predicates - ge", "")
{
    const auto pred = zx::ge(10);
    REQUIRE_THAT(  //
        zx::str(pred),
        matchers::equal_to("(ge 10)"sv));
    REQUIRE_THAT(pred(10), matchers::equal_to(true));
    REQUIRE_THAT(pred(5), matchers::equal_to(false));
    REQUIRE_THAT(pred(15), matchers::equal_to(true));
}

TEST_CASE("predicates - all", "")
{
    const auto pred = zx::all(zx::ge(10), zx::lt(15));
    REQUIRE_THAT(  //
        zx::str(pred),
        matchers::equal_to("(all (ge 10) (lt 15))"sv));
    REQUIRE_THAT(pred(9), matchers::equal_to(false));
    REQUIRE_THAT(pred(10), matchers::equal_to(true));
    REQUIRE_THAT(pred(11), matchers::equal_to(true));
    REQUIRE_THAT(pred(12), matchers::equal_to(true));
    REQUIRE_THAT(pred(13), matchers::equal_to(true));
    REQUIRE_THAT(pred(14), matchers::equal_to(true));
    REQUIRE_THAT(pred(15), matchers::equal_to(false));
    REQUIRE_THAT(pred(16), matchers::equal_to(false));
    REQUIRE_THAT(pred(17), matchers::equal_to(false));
    REQUIRE_THAT(pred(18), matchers::equal_to(false));
    REQUIRE_THAT(pred(19), matchers::equal_to(false));
    REQUIRE_THAT(pred(20), matchers::equal_to(false));
}

TEST_CASE("predicates - any", "")
{
    const auto pred = zx::any(2, 5, zx::ge(10));
    REQUIRE_THAT(  //
        zx::str(pred),
        matchers::equal_to("(any 2 5 (ge 10))"sv));
    REQUIRE_THAT(pred(2), matchers::equal_to(true));
    REQUIRE_THAT(pred(3), matchers::equal_to(false));
    REQUIRE_THAT(pred(4), matchers::equal_to(false));
    REQUIRE_THAT(pred(5), matchers::equal_to(true));
    REQUIRE_THAT(pred(9), matchers::equal_to(false));
    REQUIRE_THAT(pred(10), matchers::equal_to(true));
    REQUIRE_THAT(pred(11), matchers::equal_to(true));
    REQUIRE_THAT(pred(12), matchers::equal_to(true));
}

TEST_CASE("predicates - negate", "")
{
    const auto pred = zx::negate(zx::all(zx::ge(0), zx::lt(5)));
    REQUIRE_THAT(  //
        zx::str(pred),
        matchers::equal_to("(not (all (ge 0) (lt 5)))"sv));
    REQUIRE_THAT(pred(-1), matchers::equal_to(true));
    REQUIRE_THAT(pred(0), matchers::equal_to(false));
    REQUIRE_THAT(pred(1), matchers::equal_to(false));
    REQUIRE_THAT(pred(2), matchers::equal_to(false));
    REQUIRE_THAT(pred(3), matchers::equal_to(false));
    REQUIRE_THAT(pred(4), matchers::equal_to(false));
    REQUIRE_THAT(pred(5), matchers::equal_to(true));
}

TEST_CASE("predicates - is_some", "")
{
    const auto pred = zx::is_some(zx::ge(5));
    REQUIRE_THAT(  //
        zx::str(pred),
        matchers::equal_to("(is_some (ge 5))"sv));
    REQUIRE_THAT(pred(std::optional{ 5 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::optional{ 6 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::optional{ 4 }), matchers::equal_to(false));
    REQUIRE_THAT(pred(std::optional<int>{}), matchers::equal_to(false));
}

TEST_CASE("predicates - is_empty", "")
{
    const auto pred = zx::is_empty();
    REQUIRE_THAT(  //
        zx::str(pred),
        matchers::equal_to("(is_empty)"sv));
    REQUIRE_THAT(pred(""sv), matchers::equal_to(true));
    REQUIRE_THAT(pred("###"sv), matchers::equal_to(false));
}

TEST_CASE("predicates - size_is", "")
{
    const auto pred = zx::size_is(zx::lt(3));
    REQUIRE_THAT(  //
        zx::str(pred),
        matchers::equal_to("(size_is (lt 3))"sv));
    REQUIRE_THAT(pred(""sv), matchers::equal_to(true));
    REQUIRE_THAT(pred("#"sv), matchers::equal_to(true));
    REQUIRE_THAT(pred("##"sv), matchers::equal_to(true));
    REQUIRE_THAT(pred("###"sv), matchers::equal_to(false));
}

TEST_CASE("predicates - each_item", "")
{
    const auto pred = zx::each_item('#');
    REQUIRE_THAT(  //
        zx::str(pred),
        matchers::equal_to("(each_item #)"sv));
    REQUIRE_THAT(pred(""sv), matchers::equal_to(true));
    REQUIRE_THAT(pred("#"sv), matchers::equal_to(true));
    REQUIRE_THAT(pred("##"sv), matchers::equal_to(true));
    REQUIRE_THAT(pred("##__"sv), matchers::equal_to(false));
}

TEST_CASE("predicates - contains_item", "")
{
    const auto pred = zx::contains_item('#');
    REQUIRE_THAT(  //
        zx::str(pred),
        matchers::equal_to("(contains_item #)"sv));
    REQUIRE_THAT(pred(""sv), matchers::equal_to(false));
    REQUIRE_THAT(pred("#"sv), matchers::equal_to(true));
    REQUIRE_THAT(pred("##"sv), matchers::equal_to(true));
    REQUIRE_THAT(pred("__"sv), matchers::equal_to(false));
}

TEST_CASE("predicates - items_are", "")
{
    const auto pred = zx::items_are(0, zx::ge(3), zx::le(5), 10);
    REQUIRE_THAT(  //
        zx::str(pred),
        matchers::equal_to("(items_are 0 (ge 3) (le 5) 10)"sv));
    REQUIRE_THAT(pred(std::vector{ 0, 3, 5, 10 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::vector{ 0, 4, 4, 10 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::vector{ 0, 3, 5, 10, 100 }), matchers::equal_to(false));
    REQUIRE_THAT(pred(std::vector{ 0, 3, 5 }), matchers::equal_to(false));
}

TEST_CASE("predicates - items_are_array", "")
{
    const auto pred = zx::items_are_array(std::vector{ 1, 3, 5 });
    // REQUIRE_THAT(  //
    //     zx::str(pred),
    //     matchers::equal_to("(items_are 0 (ge 3) (le 5) 10)"sv));
    REQUIRE_THAT(pred(std::vector{ 1, 3, 5 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::vector{ 1, 3, 5, 10 }), matchers::equal_to(false));
    REQUIRE_THAT(pred(std::vector{ 1, 3 }), matchers::equal_to(false));
    REQUIRE_THAT(pred(std::vector{ 2, 2, 5 }), matchers::equal_to(false));
}

TEST_CASE("predicates - starts_with_items", "")
{
    const auto pred = zx::starts_with_items(0, zx::ge(3), zx::le(5), 10);
    REQUIRE_THAT(  //
        zx::str(pred),
        matchers::equal_to("(starts_with_items 0 (ge 3) (le 5) 10)"sv));
    REQUIRE_THAT(pred(std::vector{ 0, 3, 5, 10 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::vector{ 0, 3, 5, 10, 100 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::vector{ 0, 4, 4, 10 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::vector{ 0, 3, 5 }), matchers::equal_to(false));
}

TEST_CASE("predicates - starts_with_array", "")
{
    const auto pred = zx::starts_with_array(std::vector{ 1, 3, 5 });
    // REQUIRE_THAT(  //
    //     zx::str(pred),
    //     matchers::equal_to("(items_are 0 (ge 3) (le 5) 10)"sv));
    REQUIRE_THAT(pred(std::vector{ 1, 3, 5 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::vector{ 1, 3, 5, 10 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::vector{ 1, 3 }), matchers::equal_to(false));
    REQUIRE_THAT(pred(std::vector{ 2, 2, 5 }), matchers::equal_to(false));
}

TEST_CASE("predicates - ends_with_items", "")
{
    const auto pred = zx::ends_with_items(0, zx::ge(3), zx::le(5), 10);
    REQUIRE_THAT(  //
        zx::str(pred),
        matchers::equal_to("(ends_with_items 0 (ge 3) (le 5) 10)"sv));
    REQUIRE_THAT(pred(std::vector{ 0, 3, 5, 10 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::vector{ 100, 99, 0, 3, 5, 10 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::vector{ 0, 4, 4, 10 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::vector{ 0, 3, 5 }), matchers::equal_to(false));
}

TEST_CASE("predicates - ends_with_array", "")
{
    const auto pred = zx::ends_with_array(std::vector{ 1, 3, 5 });
    // REQUIRE_THAT(  //
    //     zx::str(pred),
    //     matchers::equal_to("(items_are 0 (ge 3) (le 5) 10)"sv));
    REQUIRE_THAT(pred(std::vector{ 1, 3, 5 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::vector{ 10, 1, 3, 5 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::vector{ 3, 5 }), matchers::equal_to(false));
    REQUIRE_THAT(pred(std::vector{ 2, 2, 5 }), matchers::equal_to(false));
}

TEST_CASE("predicates - contains_items", "")
{
    const auto pred = zx::contains_items(0, zx::ge(3), zx::le(5), 10);
    REQUIRE_THAT(  //
        zx::str(pred),
        matchers::equal_to("(contains_items 0 (ge 3) (le 5) 10)"sv));
    REQUIRE_THAT(pred(std::vector{ 0, 3, 5, 10 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::vector{ 100, 99, 0, 3, 5, 10 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::vector{ 100, 99, 0, 3, 5, 10, 200 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::vector{ 0, 4, 4, 10 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::vector{ 0, 3, 5 }), matchers::equal_to(false));
}

TEST_CASE("predicates - contains_array", "")
{
    const auto pred = zx::contains_array(std::vector{ 1, 3, 5 });
    // REQUIRE_THAT(  //
    //     zx::str(pred),
    //     matchers::equal_to("(items_are 0 (ge 3) (le 5) 10)"sv));
    REQUIRE_THAT(pred(std::vector{ 1, 3, 5 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::vector{ 10, 1, 3, 5 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::vector{ 10, 1, 3, 5, 20 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::vector{ 3, 5 }), matchers::equal_to(false));
    REQUIRE_THAT(pred(std::vector{ 2, 2, 5 }), matchers::equal_to(false));
}

TEST_CASE("predicates - result_of", "")
{
    const auto pred = zx::result_of([](const std::string& v) { return v.size(); }, zx::le(3));
    REQUIRE_THAT(  //
        zx::str(pred),
        matchers::equal_to("(result_of 1 (le 3))"sv));
    REQUIRE_THAT(pred(std::string{ "abc" }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::string{ "ab" }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::string{ "abcd" }), matchers::equal_to(false));
}

TEST_CASE("predicates - field", "")
{
    struct test_t
    {
        int field;
    };
    const auto pred = zx::field(&test_t::field, zx::le(3));
    REQUIRE_THAT(  //
        zx::str(pred),
        matchers::equal_to("(field 1 (le 3))"sv));
    REQUIRE_THAT(pred(test_t{ 3 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(test_t{ 2 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(test_t{ 12 }), matchers::equal_to(false));
}

TEST_CASE("predicates - property", "")
{
    struct test_t
    {
        int m_field;

        int field() const
        {
            return m_field;
        }
    };
    const auto pred = zx::property(&test_t::field, zx::le(3));
    REQUIRE_THAT(  //
        zx::str(pred),
        matchers::equal_to("(property 1 (le 3))"sv));
    REQUIRE_THAT(pred(test_t{ 3 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(test_t{ 2 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(test_t{ 12 }), matchers::equal_to(false));
}

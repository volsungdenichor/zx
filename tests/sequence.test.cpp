#include <zx/zx.hpp>

#include "benchmark.hpp"
#include "matchers.hpp"

auto to_roman(int number) -> std::string
{
    static const auto symbols = zx::seq::vec("M", "CM", "D", "CD", "C", "XC", "L", "XL", "X", "IX", "V", "IV", "I");
    static const auto values = zx::seq::vec(1000, 900, 500, 400, 100, 90, 50, 40, 10, 9, 5, 4, 1);

    return number > 0  //
               ? zx::seq::zip(values, symbols)
                     .drop_while(zx::pipe(zx::key, zx::gt(number)))
                     .transform(zx::destruct([&](int v, const std::string& s) { return s + to_roman(number - v); }))
                     .front()
               : "";
}

TEST_CASE("to_roman", "")
{
    REQUIRE_THAT(to_roman(1), matchers::equal_to("I"));
    REQUIRE_THAT(to_roman(2), matchers::equal_to("II"));
    REQUIRE_THAT(to_roman(3), matchers::equal_to("III"));
    REQUIRE_THAT(to_roman(4), matchers::equal_to("IV"));
    REQUIRE_THAT(to_roman(5), matchers::equal_to("V"));
    REQUIRE_THAT(to_roman(6), matchers::equal_to("VI"));
    REQUIRE_THAT(to_roman(7), matchers::equal_to("VII"));
    REQUIRE_THAT(to_roman(8), matchers::equal_to("VIII"));
    REQUIRE_THAT(to_roman(9), matchers::equal_to("IX"));
    REQUIRE_THAT(to_roman(10), matchers::equal_to("X"));
    REQUIRE_THAT(to_roman(11), matchers::equal_to("XI"));
    REQUIRE_THAT(to_roman(12), matchers::equal_to("XII"));
    REQUIRE_THAT(to_roman(13), matchers::equal_to("XIII"));
    REQUIRE_THAT(to_roman(14), matchers::equal_to("XIV"));
    REQUIRE_THAT(to_roman(19), matchers::equal_to("XIX"));
    REQUIRE_THAT(to_roman(40), matchers::equal_to("XL"));
    REQUIRE_THAT(to_roman(49), matchers::equal_to("XLIX"));
    REQUIRE_THAT(to_roman(51), matchers::equal_to("LI"));
    REQUIRE_THAT(to_roman(99), matchers::equal_to("XCIX"));
    REQUIRE_THAT(to_roman(100), matchers::equal_to("C"));
    REQUIRE_THAT(to_roman(499), matchers::equal_to("CDXCIX"));
    REQUIRE_THAT(to_roman(1998), matchers::equal_to("MCMXCVIII"));
    REQUIRE_THAT(to_roman(4000), matchers::equal_to("MMMM"));
    REQUIRE_THAT(to_roman(9000), matchers::equal_to("MMMMMMMMM"));
}

TEST_CASE("sequence - create from container", "[sequence]")
{
    std::vector<int> v = { 1, 1, 2, 3, 5, 8, 13 };
    REQUIRE_THAT(zx::sequence<int>(v), matchers::elements_are(1, 1, 2, 3, 5, 8, 13));
}

TEST_CASE("sequence - range", "[sequence]")
{
    REQUIRE_THAT(zx::seq::range(5), matchers::elements_are(0, 1, 2, 3, 4));
    REQUIRE_THAT(zx::seq::range(5, 10), matchers::elements_are(5, 6, 7, 8, 9));
}

TEST_CASE("sequence - transform", "[sequence][transform]")
{
    REQUIRE_THAT(zx::seq::range(5).transform([](int x) { return x * x; }), matchers::elements_are(0, 1, 4, 9, 16));
    REQUIRE_THAT(
        zx::seq::range(5).associate([](int x) { return x * x; }),
        matchers::elements_are(
            std::pair{ 0, 0 }, std::pair{ 1, 1 }, std::pair{ 2, 4 }, std::pair{ 3, 9 }, std::pair{ 4, 16 }));
}

TEST_CASE("sequence - filter", "[sequence][filter]")
{
    REQUIRE_THAT(zx::seq::range(5).filter([](int x) { return x % 2 == 0; }), matchers::elements_are(0, 2, 4));
}

TEST_CASE("sequence - slice", "[sequence][slicing]")
{
    REQUIRE_THAT(zx::seq::range(10).slice({ 1, 4 }), matchers::elements_are(1, 2, 3));
    REQUIRE_THAT(zx::seq::range(10).slice({ {}, 4 }), matchers::elements_are(0, 1, 2, 3));
    REQUIRE_THAT(zx::seq::range(10).slice({ 4, {} }), matchers::elements_are(4, 5, 6, 7, 8, 9));
}

TEST_CASE("sequence - associate", "[sequence][transform]")
{
    REQUIRE_THAT(
        zx::seq::range(5).associate([](int x) { return x * x; }),
        matchers::elements_are(
            std::pair{ 0, 0 }, std::pair{ 1, 1 }, std::pair{ 2, 4 }, std::pair{ 3, 9 }, std::pair{ 4, 16 }));
}

TEST_CASE("sequence - associate to map", "[sequence][transform]")
{
    const std::map<int, std::string> map = zx::seq::range(5).associate([](int x) { return zx::str('|', x * (x + 2), '|'); });
    REQUIRE_THAT(map, matchers::size_is(5));
    REQUIRE_THAT(map.at(0), matchers::equal_to("|0|"));
    REQUIRE_THAT(map.at(1), matchers::equal_to("|3|"));
    REQUIRE_THAT(map.at(2), matchers::equal_to("|8|"));
    REQUIRE_THAT(map.at(3), matchers::equal_to("|15|"));
    REQUIRE_THAT(map.at(4), matchers::equal_to("|24|"));
}

TEST_CASE("sequence - concat", "[sequence]")
{
    REQUIRE_THAT(
        zx::seq::concat(  //
            zx::seq::single("^"),
            zx::seq::range(1, 5).transform(to_roman),
            zx::seq::range(100, 105).transform(to_roman),
            zx::seq::single("$")),
        matchers::elements_are("^", "I", "II", "III", "IV", "C", "CI", "CII", "CIII", "CIV", "$"));
}

TEST_CASE("sequence - intersperse", "[sequence]")
{
    REQUIRE_THAT(zx::sequence<int>().intersperse(-1), matchers::is_empty());
    REQUIRE_THAT(zx::seq::range(100, 101).intersperse(-1), matchers::elements_are(100));
    REQUIRE_THAT(zx::seq::range(100, 105).intersperse(-1), matchers::elements_are(100, -1, 101, -1, 102, -1, 103, -1, 104));
    REQUIRE_THAT(zx::seq::view(std::string_view{ "ABC" }).intersperse(','), matchers::elements_are('A', ',', 'B', ',', 'C'));
    REQUIRE_THAT(
        zx::seq::view(std::string_view{ "ABC" })
            .transform([](char ch) -> std::string { return zx::str(ch, (char)std::tolower(ch)); })
            .intersperse(std::string{ "," })
            .transform_join(zx::seq::owning),
        matchers::elements_are('A', 'a', ',', 'B', 'b', ',', 'C', 'c'));

    REQUIRE_THAT(
        zx::seq::range(10, 15)
            .transform([](int x) { return 10 * x + 1; })
            .transform(to_roman)
            .intersperse(std::string{ ", " }),
        matchers::elements_are("CI", ", ", "CXI", ", ", "CXXI", ", ", "CXXXI", ", ", "CXLI"));

    REQUIRE_THAT(
        zx::reduce(std::string{}, std::plus<>{})(zx::seq::range(10, 15)
                                                     .transform([](int x) { return 10 * x + 1; })
                                                     .transform(to_roman)
                                                     .intersperse(std::string{ ", " })),
        matchers::equal_to("CI, CXI, CXXI, CXXXI, CXLI"));
}

#if 0
TEST_CASE("sequence - benchmark", "[sequence]")
{
    static const auto pred = zx::size_is(zx::all(zx::ge(2), zx::le(3)));
    std::cout << benchmark{}
                     .add(
                         "algorithm",
                         []()
                         {
                             volatile int i = 0;
                             std::vector<std::string> a;
                             a.reserve(5000);
                             auto input = zx::range(5000);
                             std::transform(input.begin(), input.end(), std::back_inserter(a), zx::str);
                             std::vector<std::string> b;
                             b.reserve(5000);
                             std::copy_if(a.begin(), a.end(), std::back_inserter(b), pred);
                             std::vector<std::ptrdiff_t> c;
                             c.reserve(5000);
                             std::transform(b.begin(), b.end(), std::back_inserter(c), zx::size);
                             for (auto&& v : c)
                             {
                                 i += v;
                             }
                         })
                     .add(
                         "transform-filter",
                         []()
                         {
                             volatile int i = 0;
                             for (auto&& v : zx::seq::range(5000).transform(zx::str).filter(pred).transform(zx::size))
                             {
                                 i += v;
                             }
                         })
                     .add(
                         "transform_maybe",
                         []()
                         {
                             volatile int i = 0;
                             for (auto&& v : zx::seq::range(5000).transform_maybe(
                                      [](int v) -> zx::maybe<std::ptrdiff_t>
                                      {
                                          const auto s = zx::str(v);
                                          if (!pred(s))
                                          {
                                              return {};
                                          }
                                          return zx::size(s);
                                      }))
                             {
                                 i += v;
                             }
                         })
                     .add(
                         "standard",
                         []()
                         {
                             volatile int i = 0;
                             for (int r = 0; r < 5000; ++r)
                             {
                                 const auto v = zx::str(r);
                                 if (!pred(v))
                                 {
                                     continue;
                                 }
                                 i += zx::size(v);
                             }
                         })
                     .run()
              << std::endl;
}
#endif

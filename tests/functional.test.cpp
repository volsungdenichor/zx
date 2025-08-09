#include <zx/zx.hpp>

#include "matchers.hpp"

template <class Func>
struct pipeable
{
    Func m_func;

    template <class... Args>
    constexpr auto operator()(Args&&... args) const -> std::invoke_result_t<Func, Args...>
    {
        return std::invoke(m_func, std::forward<Args>(args)...);
    }

    template <class Arg>
    constexpr friend auto operator>>=(const pipeable& p, Arg&& arg) -> std::invoke_result_t<Func, Arg>
    {
        return p(std::forward<Arg>(arg));
    }
};

template <class Func>
pipeable(Func&&) -> pipeable<std::decay_t<Func>>;

static constexpr inline struct
{
    template <class Head, class... Tail>
    auto operator()(Head head, Tail... tail) const -> Head
    {
        return head + (tail + ...);
    }
} sum;

struct join_fn
{
    template <class... Args>
    auto operator()(std::string acc, const Args&... args) const -> std::string
    {
        return acc + (acc.empty() ? "" : ", ") + zx::str(args...);
    }
};

constexpr inline auto join = pipeable{ join_fn{} };

struct arg_t
{
    template <class T>
    constexpr T&& operator=(T&& item) const
    {
        return std::forward<T>(item);
    }
};

constexpr inline auto operator""_a(const char*, std::size_t) -> arg_t
{
    return {};
}

template <class Func>
constexpr auto transform(Func func)
{
    return pipeable{ [=](auto next)
                     {
                         return [=](auto state, auto&&... args) {  //
                             return next(std::move(state), std::invoke(func, std::forward<decltype(args)>(args)...));
                         };
                     } };
}

template <class Pred>
constexpr auto filter(Pred pred)
{
    return pipeable{ [=](auto next)
                     {
                         return [=](auto state, auto&&... args) {  //
                             return std::invoke(pred, args...)     //
                                        ? next(std::move(state), std::forward<decltype(args)>(args)...)
                                        : state;
                         };
                     } };
}

TEST_CASE("reduce", "")
{
    REQUIRE_THAT(zx::reduce(0, std::plus<>{})(std::vector{ 1, 3, 5, 100 }), matchers::equal_to(109));
    REQUIRE_THAT(zx::reduce(std::string{}, join)(std::vector{ 1, 3, 5, 100 }), matchers::equal_to("1, 3, 5, 100"));

    REQUIRE_THAT(
        zx::reduce(std::string{}, transform([](int x) { return x * 11; }) >>= join)(std::vector{ 1, 3, 5, 100 }),
        matchers::equal_to("11, 33, 55, 1100"));

    constexpr auto is_even = [](int x) { return x % 2 == 0; };
    constexpr auto mul_10 = [](int x) { return x * 10; };

    REQUIRE_THAT(
        zx::reduce(         ///
            std::string{},  //
            filter(is_even) >>= transform(mul_10) >>= join)(std::vector{ 1, 2, 3, 4, 5 }),
        matchers::equal_to("20, 40"));

    REQUIRE_THAT(
        zx::reduce(         ///
            std::string{},  //
            transform(mul_10) >>= filter(is_even) >>= join)(std::vector{ 1, 2, 3, 4, 5 }),
        matchers::equal_to("10, 20, 30, 40, 50"));
}

TEST_CASE("reduce - multiple input", "")
{
    REQUIRE_THAT(zx::reduce(0, std::plus<>{})(std::vector{ 1, 3, 5, 100 }), matchers::equal_to(109));
    REQUIRE_THAT(
        zx::reduce(
            std::string{},
            [](std::string s, int v, char ch) -> std::string
            { return ch == '+' ? s + std::to_string(v) : s; })(std::vector{ 1, 3, 5, 100 }, std::string("++--")),
        matchers::equal_to("13"));

    REQUIRE_THAT(
        zx::reduce(0, [](int acc, int lt, int rt) -> int { return acc + lt * rt; })(
            std::vector{ 1, 3, 5 }, std::vector{ 10, 20, 5 }),
        matchers::equal_to(95));

    REQUIRE_THAT(
        zx::reduce(0, transform(std::multiplies<>{}) >>= sum)(std::vector{ 1, 3, 5 }, std::vector{ 10, 20, 5 }),
        matchers::equal_to(95));
}

TEST_CASE("destruct", "")
{
    REQUIRE_THAT(
        (std::tuple{ 2, 3, 4 } |= zx::destruct([](int x, int y, int z) { return x * (y + z); })), matchers::equal_to(14));
}

TEST_CASE("let", "")
{
    REQUIRE_THAT(zx::let([]() { return 3; }), matchers::equal_to(3));
    REQUIRE_THAT(zx::let("x"_a = 2 + 3, [](int x) { return 3 * x; }), matchers::equal_to(15));
    REQUIRE_THAT(
        zx::let("x"_a = 2 + 3, "y"_a = "!", [](int x, const std::string& y) { return zx::str(x, '.', y); }),
        matchers::equal_to("5.!"));
    REQUIRE_THAT(
        zx::let(
            "x"_a = 2 + 3,
            "y"_a = "!",
            "z"_a = 'x',
            [](int x, const std::string& y, char z) { return zx::str(x, '.', y, '.', z); }),
        matchers::equal_to("5.!.x"));
    REQUIRE_THAT(
        zx::let(
            "vect"_a = std::vector<int>{ 2, 5, 13, 19 },
            "ctrl"_a = std::tuple{ ", ", "[", "]" },
            [](const auto& vect, const auto& ctrl) -> std::string
            {
                const auto& [sep, open, close] = ctrl;
                return zx::str(open, zx::delimit(vect, sep), close);
            }),
        matchers::equal_to("[2, 5, 13, 19]"));
}

TEST_CASE("proj", "")
{
    REQUIRE_THAT(zx::proj(zx::size, std::plus<>{})(std::vector{ 1, 2, 3 }, std::array{ 1, 2, 3, 4 }), matchers::equal_to(7));
    REQUIRE_THAT(
        zx::proj(zx::size, std::equal_to<>{})(std::vector{ 1, 2, 3 }, std::array{ 1, 2, 3 }), matchers::equal_to(true));
    REQUIRE_THAT(
        zx::proj(
            [](const std::vector<int>& v) {
                return std::tuple{ v.front(), v.back(), v.size() };
            },
            zx::str)(std::vector{ 3, 5, 7 }, std::vector{ 9, 10, 11 }),
        matchers::equal_to("(3, 7, 3)(9, 11, 3)"));
}
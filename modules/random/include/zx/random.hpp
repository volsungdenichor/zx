#pragma once

#include <functional>
#include <optional>
#include <random>
#include <tuple>

namespace zx
{
namespace random
{

struct seed_t
{
    using result_type = std::mt19937::result_type;

    seed_t() = default;
    explicit seed_t(result_type value) : m_value{ value } { }

    result_type operator()() const { return m_value ? *m_value : std::random_device{}(); }

    std::optional<result_type> m_value;
};

namespace detail
{

struct uniform_fn
{
    template <class T, class = void>
    struct impl_t;

    template <class T>
    struct impl_t<T, std::enable_if_t<std::is_floating_point_v<T>>>
    {
        impl_t(T lo, T up, seed_t seed = seed_t{}) : m_rng{ seed() }, m_dist{ lo, up } { }

        T operator()() const { return m_dist(m_rng); }

    private:
        mutable std::mt19937 m_rng;
        mutable std::uniform_real_distribution<T> m_dist;
    };

    template <class T>
    struct impl_t<T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>>>
    {
        impl_t(T lo, T up, seed_t seed = seed_t{}) : m_rng{ seed() }, m_dist{ lo, up } { }

        T operator()() const { return m_dist(m_rng); }

    private:
        mutable std::mt19937 m_rng;
        mutable std::uniform_int_distribution<T> m_dist;
    };

    template <class T>
    impl_t<T> operator()(T lo, T up, seed_t seed = seed_t{}) const
    {
        return impl_t<T>{ lo, up, seed };
    }
};

struct invoke_fn
{
    template <class Func, class... Args>
    struct impl_t
    {
        Func m_func;
        std::tuple<Args...> m_args;

        auto operator()() const
        {
            return std::apply([&](const auto&... args) { return std::invoke(m_func, args()...); }, m_args);
        }
    };

    template <class Func, class... Args>
    impl_t<std::decay_t<Func>, std::decay_t<Args>...> operator()(Func&& func, Args&&... args) const
    {
        return impl_t<std::decay_t<Func>, std::decay_t<Args>...>{ std::forward<Func>(func),
                                                                  { std::forward<Args>(args)... } };
    }
};

}  // namespace detail

constexpr inline auto uniform = detail::uniform_fn{};
constexpr inline auto invoke = detail::invoke_fn{};

}  // namespace random
}  // namespace zx

#pragma once

#include <algorithm>
#include <array>
#include <functional>
#include <iostream>
#include <zx/type_traits.hpp>

namespace zx
{

namespace mat
{

constexpr struct eq_fn
{
    template <class... Its, std::size_t... Is>
    bool operator()(const std::tuple<Its...>& lhs, const std::tuple<Its...>& rhs, std::index_sequence<Is...>) const
    {
        return ((std::get<Is>(lhs) == std::get<Is>(rhs)) && ...);
    }

    template <class... Its>
    bool operator()(const std::tuple<Its...>& lhs, const std::tuple<Its...>& rhs) const
    {
        return (*this)(lhs, rhs, std::make_index_sequence<sizeof...(Its)>{});
    }
} eq = {};

constexpr struct inc_fn
{
    template <class... Its, std::size_t... Is>
    void operator()(std::tuple<Its...>& it, std::index_sequence<Is...>) const
    {
        ((++std::get<Is>(it)), ...);
    }

    template <class... Its>
    void operator()(std::tuple<Its...>& it) const
    {
        (*this)(it, std::make_index_sequence<sizeof...(Its)>{});
    }
} inc = {};

template <class Func, class Out, class... Ranges>
constexpr Out transform_into(Out out, Func func, Ranges&&... ranges)
{
    auto it = std::tuple{ std::begin(ranges)... };
    const auto end = std::tuple{ std::end(ranges)... };
    auto b = std::begin(out);
    for (; !eq(it, end); inc(it), ++b)
    {
        *b = std::apply([&](auto... iters) { return std::invoke(func, *iters...); }, it);
    }
    return out;
}

template <class Func, class InOut, class... Ranges>
constexpr InOut& transform(Func func, InOut& in_out, Ranges&&... ranges)
{
    auto it = std::tuple{ std::begin(ranges)... };
    const auto end = std::tuple{ std::end(ranges)... };
    auto b = std::begin(in_out);
    for (; !eq(it, end); inc(it), ++b)
    {
        *b = std::apply([&](auto... iters) { return std::invoke(func, *b, *iters...); }, it);
    }
    return in_out;
}

template <class Op, class Arg>
constexpr auto bind_back(Op op, Arg arg)
{
    return std::bind(std::move(op), std::placeholders::_1, std::move(arg));
}

template <std::size_t D, class T, template <std::size_t, class...> class Self>
struct md_base_t : public std::array<T, D>
{
    using base_t = std::array<T, D>;
    using self_type = Self<D, T>;
    using base_t::base_t;

    constexpr md_base_t() : base_t{} { std::fill(this->begin(), this->end(), T{}); }

    template <class... Tail>
    constexpr md_base_t(T head, Tail... tail) : base_t{ head, static_cast<T>(tail)... }
    {
        static_assert(sizeof...(tail) + 1 == D, "Invalid number of arguments to md_base_t constructor");
    }

    constexpr explicit md_base_t(const base_t& other) : base_t(other) { }

    friend std::ostream& operator<<(std::ostream& os, const md_base_t& item)
    {
        os << "[";
        for (std::size_t i = 0; i < D; ++i)
        {
            if (i != 0)
            {
                os << " ";
            }
            os << item[i];
        }
        os << "]";
        return os;
    }

    friend bool operator==(const md_base_t& lhs, const md_base_t& rhs)
    {
        return std::equal(lhs.begin(), lhs.end(), rhs.begin());
    }

    friend bool operator!=(const md_base_t& lhs, const md_base_t& rhs) { return !(lhs == rhs); }
};

template <class T>
struct is_vector : public std::false_type
{
};

template <class T>
struct is_matrix : public std::false_type
{
};

template <class T>
struct is_scalar : public std::bool_constant<!is_vector<T>::value && !is_matrix<T>::value>
{
};

template <std::size_t D, class T>
struct vector_t;

template <std::size_t D, class T>
struct vector_t : public md_base_t<D, T, vector_t>
{
    using base_t = md_base_t<D, T, vector_t>;

    using base_t::base_t;
};

template <std::size_t D, class T>
struct is_vector<vector_t<D, T>> : public std::true_type
{
};

template <class T>
vector_t(T, T) -> vector_t<2, T>;

template <class T>
vector_t(T, T, T) -> vector_t<3, T>;

namespace detail
{

struct vector_fn
{
    template <class T>
    constexpr auto operator()(T x, T y) const -> vector_t<2, T>
    {
        return { x, y };
    }

    template <class T>
    constexpr auto operator()(T x, T y, T z) const -> vector_t<3, T>
    {
        return { x, y, z };
    }
};

}  // namespace detail

static constexpr inline auto vector = detail::vector_fn{};

template <std::size_t D, class T>
constexpr auto operator+(const vector_t<D, T>& item) -> vector_t<D, T>
{
    return item;
}

template <std::size_t D, class T>
constexpr auto operator-(const vector_t<D, T>& item) -> vector_t<D, T>
{
    return transform_into(vector_t<D, T>{}, std::negate<>{}, item);
}

template <std::size_t D, class L, class R, class Res = std::invoke_result_t<std::plus<>, L, R>>
constexpr auto operator+=(vector_t<D, L>& lhs, const vector_t<D, R>& rhs) -> vector_t<D, L>&
{
    return transform(std::plus<>{}, lhs, rhs);
}

template <std::size_t D, class L, class R, class Res = std::invoke_result_t<std::plus<>, L, R>>
constexpr auto operator+(const vector_t<D, L>& lhs, const vector_t<D, R>& rhs) -> vector_t<D, Res>
{
    return transform_into(vector_t<D, Res>{}, std::plus<>{}, lhs, rhs);
}

template <std::size_t D, class L, class R, class Res = std::invoke_result_t<std::minus<>, L, R>>
constexpr auto operator-=(vector_t<D, L>& lhs, const vector_t<D, R>& rhs) -> vector_t<D, L>&
{
    return transform(std::minus<>{}, lhs, rhs);
}

template <std::size_t D, class L, class R, class Res = std::invoke_result_t<std::minus<>, L, R>>
constexpr auto operator-(const vector_t<D, L>& lhs, const vector_t<D, R>& rhs) -> vector_t<D, Res>
{
    return transform_into(vector_t<D, Res>{}, std::minus<>{}, lhs, rhs);
}

template <
    std::size_t D,
    class L,
    class R,
    enable_if_t<is_scalar<R>::value> = 0,
    class Res = std::invoke_result_t<std::multiplies<>, L, R>>
constexpr auto operator*=(vector_t<D, L>& lhs, R rhs) -> vector_t<D, L>&
{
    return transform(bind_back(std::multiplies<>{}, rhs), lhs);
}

template <
    std::size_t D,
    class L,
    class R,
    enable_if_t<is_scalar<R>::value> = 0,
    class Res = std::invoke_result_t<std::multiplies<>, L, R>>
constexpr auto operator*(const vector_t<D, L>& lhs, R rhs) -> vector_t<D, Res>
{
    return transform_into(vector_t<D, Res>{}, bind_back(std::multiplies<>{}, rhs), lhs);
}

template <
    class L,
    std::size_t D,
    class R,
    enable_if_t<is_scalar<L>::value> = 0,
    class Res = std::invoke_result_t<std::multiplies<>, L, R>>
constexpr auto operator*(L lhs, const vector_t<D, R>& rhs) -> vector_t<D, Res>
{
    return rhs * lhs;
}

template <
    std::size_t D,
    class L,
    class R,
    enable_if_t<is_scalar<R>::value> = 0,
    class Res = std::invoke_result_t<std::divides<>, L, R>>
constexpr auto operator/=(vector_t<D, L>& lhs, R rhs) -> vector_t<D, L>&
{
    return transform(bind_back(std::divides<>{}, rhs), lhs);
}

template <
    std::size_t D,
    class L,
    class R,
    enable_if_t<is_scalar<R>::value> = 0,
    class Res = std::invoke_result_t<std::divides<>, L, R>>
constexpr auto operator/(const vector_t<D, L>& lhs, R rhs) -> vector_t<D, Res>
{
    return transform_into(vector_t<D, Res>{}, bind_back(std::divides<>{}, rhs), lhs);
}

template <std::size_t D, class L, class R, class = std::invoke_result_t<std::equal_to<>, L, R>>
constexpr bool operator==(const vector_t<D, L>& lhs, const vector_t<D, R>& rhs)
{
    return std::equal(std::begin(lhs), std::end(lhs), std::begin(rhs));
}

template <std::size_t D, class L, class R, class = std::invoke_result_t<std::equal_to<>, L, R>>
constexpr bool operator!=(const vector_t<D, L>& lhs, const vector_t<D, R>& rhs)
{
    return !(lhs == rhs);
}

}  // namespace mat
}  // namespace zx

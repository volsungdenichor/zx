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

namespace detail
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
constexpr Out map_into(Out out, Func func, Ranges&&... ranges)
{
    auto begin = std::tuple{ std::begin(out), std::begin(ranges)... };
    const auto end = std::tuple{ std::end(out), std::end(ranges)... };
    for (; !eq(begin, end); inc(begin))
    {
        std::apply([&](auto in, auto... iters) { *in = std::invoke(func, *iters...); }, begin);
    }
    return out;
}

template <class Func, class InOut, class... Ranges>
constexpr InOut& map(Func func, InOut& in_out, Ranges&&... ranges)
{
    auto begin = std::tuple{ std::begin(in_out), std::begin(ranges)... };
    const auto end = std::tuple{ std::end(in_out), std::end(ranges)... };
    for (; !eq(begin, end); inc(begin))
    {
        std::apply(
            [&](auto head, auto... tail)
            {
                auto& out = *head;
                out = std::invoke(func, out, *tail...);
            },
            begin);
    }
    return in_out;
}

template <class Op, class Arg>
constexpr auto bind_back(Op op, Arg arg)
{
    return std::bind(std::move(op), std::placeholders::_1, std::move(arg));
}

}  // namespace detail

template <std::size_t D, class T, template <std::size_t, class...> class Self>
struct md_base_t : public std::array<T, D>
{
    using base_t = std::array<T, D>;
    using self_type = Self<D, T>;
    using base_t::base_t;

    constexpr md_base_t() : base_t{} { std::fill(this->begin(), this->end(), T{}); }

    template <std::size_t D_ = D, enable_if_t<(D_ == 1)> = 0>
    constexpr md_base_t(T v0) : base_t{ v0 }
    {
    }

    template <std::size_t D_ = D, enable_if_t<(D_ == 2)> = 0>
    constexpr md_base_t(T v0, T v1) : base_t{ v0, v1 }
    {
    }

    template <std::size_t D_ = D, enable_if_t<(D_ == 3)> = 0>
    constexpr md_base_t(T v0, T v1, T v2) : base_t{ v0, v1, v2 }
    {
    }

    template <std::size_t D_ = D, enable_if_t<(D_ == 4)> = 0>
    constexpr md_base_t(T v0, T v1, T v2, T v3) : base_t{ v0, v1, v2, v3 }
    {
    }

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

    friend bool operator==(const self_type& lhs, const self_type& rhs)
    {
        return std::equal(lhs.begin(), lhs.end(), rhs.begin());
    }

    friend bool operator!=(const self_type& lhs, const self_type& rhs) { return !(lhs == rhs); }
};

template <std::size_t D, class T>
struct vector_t : public md_base_t<D, T, vector_t>
{
    using base_t = md_base_t<D, T, vector_t>;

    using base_t::base_t;

    template <class U>
    vector_t<D, U> to() const
    {
        vector_t<D, U> result;
        for (std::size_t d = 0; d < D; ++d)
        {
            result[d] = static_cast<U>((*this)[d]);
        }
        return result;
    }

    static constexpr vector_t zeros() { return create_uniform<0>(); }
    static constexpr vector_t ones() { return create_uniform<1>(); }

    template <int V>
    static constexpr vector_t create_uniform()
    {
        vector_t result;
        for (std::size_t d = 0; d < D; ++d)
        {
            result[d] = static_cast<T>(V);
        }
        return result;
    }
};

template <std::size_t D, class T>
constexpr auto insert(const vector_t<D, T>& v, std::size_t index, T value) -> vector_t<D + 1, T>
{
    vector_t<D + 1, T> result;
    for (std::size_t d = 0; d < index; ++d)
    {
        result[d] = v[d];
    }
    result[index] = value;
    for (std::size_t d = index; d < D; ++d)
    {
        result[d + 1] = v[d];
    }
    return result;
}

template <std::size_t D, class T, enable_if_t<(D > 1)> = 0>
constexpr auto erase(const vector_t<D, T>& v, std::size_t index) -> vector_t<D - 1, T>
{
    vector_t<D - 1, T> result;
    for (std::size_t d = 0; d < index; ++d)
    {
        result[d] = v[d];
    }
    for (std::size_t d = index + 1; d < D; ++d)
    {
        result[d - 1] = v[d];
    }
    return result;
}

template <std::size_t D, class T>
constexpr auto append(const vector_t<D, T>& v, T value) -> vector_t<D + 1, T>
{
    return insert(v, D, value);
}

template <std::size_t D, class T>
constexpr auto prepend(const vector_t<D, T>& v, T value) -> vector_t<D + 1, T>
{
    return insert(v, 0, value);
}

template <std::size_t D, class T>
constexpr auto pop_back(const vector_t<D, T>& v) -> vector_t<D - 1, T>
{
    return erase(v, D - 1);
}

template <std::size_t D, class T>
constexpr auto pop_front(const vector_t<D, T>& v) -> vector_t<D - 1, T>
{
    return erase(v, 0);
}

template <std::size_t D, class T>
using point_t = vector_t<D, T>;

template <std::size_t D, class T>
using extent_t = vector_t<D, T>;

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
        return vector_t<2, T>{ x, y };
    }

    template <class T>
    constexpr auto operator()(T x, T y, T z) const -> vector_t<3, T>
    {
        return vector_t<3, T>{ x, y, z };
    }
};

}  // namespace detail

static constexpr inline auto vector = detail::vector_fn{};
static constexpr inline auto point = detail::vector_fn{};
static constexpr inline auto extent = detail::vector_fn{};

template <std::size_t D, class T>
constexpr auto operator+(const vector_t<D, T>& item) -> vector_t<D, T>
{
    return item;
}

template <std::size_t D, class T>
constexpr auto operator-(const vector_t<D, T>& item) -> vector_t<D, T>
{
    return detail::map_into(vector_t<D, T>{}, std::negate<>{}, item);
}

template <std::size_t D, class L, class R, class Res = std::invoke_result_t<std::plus<>, L, R>>
constexpr auto operator+=(vector_t<D, L>& lhs, const vector_t<D, R>& rhs) -> vector_t<D, L>&
{
    return detail::map(std::plus<>{}, lhs, rhs);
}

template <std::size_t D, class L, class R, class Res = std::invoke_result_t<std::plus<>, L, R>>
constexpr auto operator+(const vector_t<D, L>& lhs, const vector_t<D, R>& rhs) -> vector_t<D, Res>
{
    return detail::map_into(vector_t<D, Res>{}, std::plus<>{}, lhs, rhs);
}

template <std::size_t D, class L, class R, class Res = std::invoke_result_t<std::minus<>, L, R>>
constexpr auto operator-=(vector_t<D, L>& lhs, const vector_t<D, R>& rhs) -> vector_t<D, L>&
{
    return detail::map(std::minus<>{}, lhs, rhs);
}

template <std::size_t D, class L, class R, class Res = std::invoke_result_t<std::minus<>, L, R>>
constexpr auto operator-(const vector_t<D, L>& lhs, const vector_t<D, R>& rhs) -> vector_t<D, Res>
{
    return detail::map_into(vector_t<D, Res>{}, std::minus<>{}, lhs, rhs);
}

template <std::size_t D, class L, class R, class Res = std::invoke_result_t<std::multiplies<>, L, R>>
constexpr auto operator*=(vector_t<D, L>& lhs, R rhs) -> vector_t<D, L>&
{
    return detail::map(detail::bind_back(std::multiplies<>{}, rhs), lhs);
}

template <std::size_t D, class L, class R, class Res = std::invoke_result_t<std::multiplies<>, L, R>>
constexpr auto operator*(const vector_t<D, L>& lhs, R rhs) -> vector_t<D, Res>
{
    return detail::map_into(vector_t<D, Res>{}, detail::bind_back(std::multiplies<>{}, rhs), lhs);
}

template <class L, std::size_t D, class R, class Res = std::invoke_result_t<std::multiplies<>, L, R>>
constexpr auto operator*(L lhs, const vector_t<D, R>& rhs) -> vector_t<D, Res>
{
    return rhs * lhs;
}

template <std::size_t D, class L, class R, class Res = std::invoke_result_t<std::divides<>, L, R>>
constexpr auto operator/=(vector_t<D, L>& lhs, R rhs) -> vector_t<D, L>&
{
    return detail::map(detail::bind_back(std::divides<>{}, rhs), lhs);
}

template <std::size_t D, class L, class R, class Res = std::invoke_result_t<std::divides<>, L, R>>
constexpr auto operator/(const vector_t<D, L>& lhs, R rhs) -> vector_t<D, Res>
{
    return detail::map_into(vector_t<D, Res>{}, detail::bind_back(std::divides<>{}, rhs), lhs);
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

namespace std
{

template <size_t D, class T>
struct tuple_size<zx::mat::vector_t<D, T>> : integral_constant<size_t, D>
{
};

template <size_t I, size_t D, class T>
struct tuple_element<I, zx::mat::vector_t<D, T>>
{
    using type = T;
};

template <size_t I, size_t D, class T>
constexpr T& get(zx::mat::vector_t<D, T>& item) noexcept
{
    return item[I];
}

template <size_t I, size_t D, class T>
constexpr const T& get(const zx::mat::vector_t<D, T>& item) noexcept
{
    return item[I];
}

template <size_t I, size_t D, class T>
constexpr T&& get(zx::mat::vector_t<D, T>&& item) noexcept
{
    return std::move(item[I]);
}

}  // namespace std

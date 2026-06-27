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

    template <class Func, class Res = std::invoke_result_t<Func, T>>
    constexpr std::array<Res, D> transform(Func func) const
    {
        std::array<Res, D> result;
        std::transform(std::begin(*this), std::end(*this), std::begin(result), func);
        return result;
    }

    template <class Res, class Func>
    constexpr Res transform_to(Func func) const
    {
        Res result;
        std::transform(std::begin(*this), std::end(*this), std::begin(result), func);
        return result;
    }

    template <class Res, class Func, class Other>
    Res transform_to(Func func, Other&& other) const
    {
        Res result;
        std::transform(std::begin(*this), std::end(*this), std::begin(other), std::begin(result), func);
        return result;
    }

    template <class Func>
    self_type& transform_self(Func func)
    {
        std::transform(std::begin(*this), std::end(*this), std::begin(*this), func);
        return static_cast<self_type&>(*this);
    }

    template <class Func, class Other>
    self_type& transform_self(Func func, Other&& other)
    {
        std::transform(std::begin(*this), std::end(*this), std::begin(other), std::begin(*this), func);
        return static_cast<self_type&>(*this);
    }
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
    return item.template transform_to<vector_t<D, T>>(std::negate<>{});
}

template <std::size_t D, class L, class R, class Res = std::invoke_result_t<std::plus<>, L, R>>
constexpr auto operator+=(vector_t<D, L>& lhs, const vector_t<D, R>& rhs) -> vector_t<D, L>&
{
    return lhs.transform_self(std::plus<>{}, rhs);
}

template <std::size_t D, class L, class R, class Res = std::invoke_result_t<std::plus<>, L, R>>
constexpr auto operator+(const vector_t<D, L>& lhs, const vector_t<D, R>& rhs) -> vector_t<D, Res>
{
    return lhs.template transform_to<vector_t<D, Res>>(std::plus<>{}, rhs);
}

template <std::size_t D, class L, class R, class Res = std::invoke_result_t<std::minus<>, L, R>>
constexpr auto operator-=(vector_t<D, L>& lhs, const vector_t<D, R>& rhs) -> vector_t<D, L>&
{
    return lhs.transform_self(std::minus<>{}, rhs);
}

template <std::size_t D, class L, class R, class Res = std::invoke_result_t<std::minus<>, L, R>>
constexpr auto operator-(const vector_t<D, L>& lhs, const vector_t<D, R>& rhs) -> vector_t<D, Res>
{
    return lhs.template transform_to<vector_t<D, Res>>(std::minus<>{}, rhs);
}

template <
    std::size_t D,
    class L,
    class R,
    class Res = std::invoke_result_t<std::multiplies<>, L, R>,
    class = std::enable_if_t<is_scalar<R>::value>>
constexpr auto operator*=(vector_t<D, L>& lhs, R rhs) -> vector_t<D, L>&
{
    return lhs.transform_self([&](const L& value) -> L { return value * rhs; });
}

template <
    std::size_t D,
    class L,
    class R,
    class Res = std::invoke_result_t<std::multiplies<>, L, R>,
    class = std::enable_if_t<is_scalar<R>::value>>
constexpr auto operator*(const vector_t<D, L>& lhs, R rhs) -> vector_t<D, Res>
{
    return lhs.template transform_to<vector_t<D, Res>>([&](const L& value) -> Res { return value * rhs; });
}

template <
    class L,
    std::size_t D,
    class R,
    class Res = std::invoke_result_t<std::multiplies<>, L, R>,
    class = std::enable_if_t<is_scalar<L>::value>>
constexpr auto operator*(L lhs, const vector_t<D, R>& rhs) -> vector_t<D, Res>
{
    return rhs * lhs;
}

template <
    std::size_t D,
    class L,
    class R,
    class Res = std::invoke_result_t<std::divides<>, L, R>,
    class = std::enable_if_t<is_scalar<R>::value>>
constexpr auto operator/=(vector_t<D, L>& lhs, R rhs) -> vector_t<D, L>&
{
    return lhs.transform_self([&](const L& value) -> L { return value / rhs; });
}

template <
    std::size_t D,
    class L,
    class R,
    class Res = std::invoke_result_t<std::divides<>, L, R>,
    class = std::enable_if_t<is_scalar<R>::value>>
constexpr auto operator/(const vector_t<D, L>& lhs, R rhs) -> vector_t<D, Res>
{
    return lhs.template transform_to<vector_t<D, Res>>([&](const L& value) -> Res { return value / rhs; });
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

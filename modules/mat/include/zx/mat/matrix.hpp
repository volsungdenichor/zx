#pragma once

#include <limits>
#include <numeric>
#include <optional>
#include <zx/iterator_range.hpp>
#include <zx/mat/math.hpp>
#include <zx/mat/vector.hpp>
#include <zx/strided_iterator.hpp>

namespace zx
{

namespace mat
{
template <std::size_t R, std::size_t C, class T>
struct matrix_view_t
{
    using size_type = std::size_t;
    using volume_type = std::size_t;

    using extents_type = vector_t<2, size_type>;
    using location_type = vector_t<2, size_type>;

    using pointer = T*;
    using reference = T&;
    using row_type = iterator_range_t<pointer>;
    using column_type = iterator_range_t<strided_iterator<pointer, std::ptrdiff_t(C)>>;
    using data_type = iterator_range_t<pointer>;

    pointer m_data;

    constexpr matrix_view_t(pointer data) : m_data(data) { }

    constexpr size_type row_count() const { return R; }

    constexpr size_type column_count() const { return C; }

    constexpr extents_type extents() const { return extents_type{ row_count(), column_count() }; }

    constexpr volume_type volume() const { return row_count() * column_count(); }

    constexpr data_type data() const { return data_type(m_data, volume()); }

    constexpr reference operator[](size_type n) const { return data()[n]; }

    constexpr reference operator[](const location_type& loc) const { return *(m_data + loc[0] * column_count() + loc[1]); }

    constexpr row_type row(size_type n) const
    {
        return row_type(
            typename row_type::iterator{ m_data + n * column_count() }, static_cast<std::ptrdiff_t>(column_count()));
    }

    constexpr column_type column(size_type n) const
    {
        return column_type(typename column_type::iterator{ m_data + n }, static_cast<std::ptrdiff_t>(row_count()));
    }
};

template <std::size_t R, std::size_t C, class T>
struct matrix_t
{
    using view_type = matrix_view_t<R, C, T>;
    using const_view_type = matrix_view_t<R, C, const T>;

    using size_type = typename view_type::size_type;
    using volume_type = typename view_type::volume_type;

    using extents_type = typename view_type::extents_type;
    using location_type = typename view_type::location_type;

    using pointer = typename view_type::pointer;
    using reference = typename view_type::reference;
    using row_type = typename view_type::row_type;
    using column_type = typename view_type::column_type;
    using data_type = typename view_type::data_type;

    using const_pointer = typename const_view_type::pointer;
    using const_reference = typename const_view_type::reference;
    using const_row_type = typename const_view_type::row_type;
    using const_column_type = typename const_view_type::column_type;
    using const_data_type = typename const_view_type::data_type;

    std::array<T, R * C> m_data;

    constexpr matrix_t() : m_data{} { }

    constexpr matrix_t(std::initializer_list<T> init) : matrix_t{}
    {
        std::copy(std::begin(init), std::end(init), std::begin(m_data));
    }

    constexpr view_type view() { return view_type{ m_data.data() }; }

    constexpr const_view_type view() const { return const_view_type{ m_data.data() }; }

    constexpr size_type row_count() const { return view().row_count(); }

    constexpr size_type column_count() const { return view().column_count(); }

    constexpr volume_type volume() const { return view().volume(); }

    constexpr extents_type extents() const { return view().extents(); }

    constexpr data_type data() { return view().data(); }

    constexpr const_data_type data() const { return view().data(); }

    constexpr reference operator[](size_type n) { return view()[n]; }

    constexpr const_reference operator[](size_type n) const { return view()[n]; }

    constexpr reference operator[](const location_type& loc) { return view()[loc]; }

    constexpr const_reference operator[](const location_type& loc) const { return view()[loc]; }

    constexpr row_type row(size_type n) { return view().row(n); }

    constexpr const_row_type row(size_type n) const { return view().row(n); }

    constexpr column_type column(size_type n) { return view().column(n); }

    constexpr const_column_type column(size_type n) const { return view().column(n); }
};

template <std::size_t R, std::size_t C, class T>
struct is_matrix<matrix_t<R, C, T>> : public std::true_type
{
};

template <std::size_t R, std::size_t C, class T>
std::ostream& operator<<(std::ostream& os, const matrix_view_t<R, C, T>& item)
{
    os << "[";
    for (std::size_t r = 0; r < item.row_count(); ++r)
    {
        const auto row = item.row(r);
        if (r != 0)
        {
            os << " ";
        }
        os << "[";
        for (std::size_t c = 0; c < item.column_count(); ++c)
        {
            if (c != 0)
            {
                os << " ";
            }
            os << row[static_cast<std::ptrdiff_t>(c)];
        }
        os << "]";
    }
    os << "]";
    return os;
}

template <std::size_t R, std::size_t C, class T>
std::ostream& operator<<(std::ostream& os, const matrix_t<R, C, T>& item)
{
    return os << item.view();
}

template <std::size_t R, std::size_t C, class T>
constexpr auto operator+(const matrix_t<R, C, T>& item) -> matrix_t<R, C, T>
{
    return item;
}

template <std::size_t R, std::size_t C, class T>
constexpr auto operator-(const matrix_t<R, C, T>& item) -> matrix_t<R, C, T>
{
    matrix_t<R, C, T> result{};
    std::transform(std::begin(item.data()), std::end(item.data()), std::begin(result.data()), std::negate<>{});
    return result;
}

template <std::size_t R, std::size_t C, class T, class U, class = std::invoke_result_t<std::plus<>, T, U>>
constexpr auto operator+=(matrix_t<R, C, T>& lhs, const matrix_t<R, C, U>& rhs) -> matrix_t<R, C, T>&
{
    std::transform(
        std::begin(lhs.data()), std::end(lhs.data()), std::begin(rhs.data()), std::begin(lhs.data()), std::plus<>{});
    return lhs;
}

template <std::size_t R, std::size_t C, class T, class U, class = std::invoke_result_t<std::minus<>, T, U>>
constexpr auto operator-=(matrix_t<R, C, T>& lhs, const matrix_t<R, C, U>& rhs) -> matrix_t<R, C, T>&
{
    std::transform(
        std::begin(lhs.data()), std::end(lhs.data()), std::begin(rhs.data()), std::begin(lhs.data()), std::minus<>{});
    return lhs;
}

template <
    std::size_t R,
    std::size_t C,
    class T,
    class U,
    class = std::invoke_result_t<std::multiplies<>, T, U>,
    class = std::enable_if_t<is_scalar<U>::value>>
constexpr auto operator*=(matrix_t<R, C, T>& lhs, U rhs) -> matrix_t<R, C, T>&
{
    std::transform(
        std::begin(lhs.data()),
        std::end(lhs.data()),
        std::begin(lhs.data()),
        std::bind(std::multiplies<>{}, std::placeholders::_1, rhs));
    return lhs;
}

template <
    std::size_t R,
    std::size_t C,
    class T,
    class U,
    class = std::invoke_result_t<std::divides<>, T, U>,
    class = std::enable_if_t<is_scalar<U>::value>>
constexpr auto operator/=(matrix_t<R, C, T>& lhs, U rhs) -> matrix_t<R, C, T>&
{
    std::transform(
        std::begin(lhs.data()),
        std::end(lhs.data()),
        std::begin(lhs.data()),
        std::bind(std::divides<>{}, std::placeholders::_1, rhs));
    return lhs;
}

template <std::size_t R, std::size_t C, class T, class U, class Res = std::invoke_result_t<std::plus<>, T, U>>
constexpr auto operator+(const matrix_t<R, C, T>& lhs, const matrix_t<R, C, U>& rhs) -> matrix_t<R, C, Res>
{
    matrix_t<R, C, Res> result{};
    std::transform(
        std::begin(lhs.data()), std::end(lhs.data()), std::begin(rhs.data()), std::begin(result.data()), std::plus<>{});
    return result;
}

template <std::size_t R, std::size_t C, class T, class U, class Res = std::invoke_result_t<std::minus<>, T, U>>
constexpr auto operator-(const matrix_t<R, C, T>& lhs, const matrix_t<R, C, U>& rhs) -> matrix_t<R, C, Res>
{
    matrix_t<R, C, Res> result{};
    std::transform(
        std::begin(lhs.data()), std::end(lhs.data()), std::begin(rhs.data()), std::begin(result.data()), std::minus<>{});
    return result;
}

template <
    std::size_t R,
    std::size_t C,
    class T,
    class U,
    class Res = std::invoke_result_t<std::multiplies<>, T, U>,
    class = std::enable_if_t<is_scalar<U>::value>>
constexpr auto operator*(const matrix_t<R, C, T>& lhs, U rhs) -> matrix_t<R, C, Res>
{
    matrix_t<R, C, Res> result{};
    std::transform(
        std::begin(lhs.data()),
        std::end(lhs.data()),
        std::begin(result.data()),
        std::bind(std::multiplies<>{}, std::placeholders::_1, rhs));
    return result;
}

template <
    std::size_t R,
    std::size_t C,
    class T,
    class U,
    class Res = std::invoke_result_t<std::multiplies<>, T, U>,
    class = std::enable_if_t<is_scalar<T>::value>>
constexpr auto operator*(T lhs, const matrix_t<R, C, U>& rhs) -> matrix_t<R, C, Res>
{
    return rhs * lhs;
}

template <
    std::size_t R,
    std::size_t C,
    class T,
    class U,
    class Res = std::invoke_result_t<std::divides<>, T, U>,
    class = std::enable_if_t<is_scalar<U>::value>>
constexpr auto operator/(const matrix_t<R, C, T>& lhs, U rhs) -> matrix_t<R, C, Res>
{
    matrix_t<R, C, Res> result{};
    std::transform(
        std::begin(lhs.data()),
        std::end(lhs.data()),
        std::begin(result.data()),
        std::bind(std::divides<>{}, std::placeholders::_1, rhs));
    return result;
}

template <std::size_t R, std::size_t C, class T, class U, class = std::invoke_result_t<std::equal_to<>, T, U>>
constexpr bool operator==(const matrix_t<R, C, T>& lhs, const matrix_t<R, C, U>& rhs)
{
    return std::equal(std::begin(lhs.data()), std::end(lhs.data()), std::begin(rhs.data()));
}

template <std::size_t R, std::size_t C, class T, class U, class = std::invoke_result_t<std::equal_to<>, T, U>>
constexpr bool operator!=(const matrix_t<R, C, T>& lhs, const matrix_t<R, C, U>& rhs)
{
    return !(lhs == rhs);
}

template <
    std::size_t R,
    std::size_t D,
    std::size_t C,
    class T,
    class U,
    class Res = std::invoke_result_t<std::multiplies<>, T, U>>
constexpr auto operator*(const matrix_t<R, D, T>& lhs, const matrix_t<D, C, U>& rhs) -> matrix_t<R, C, Res>
{
    matrix_t<R, C, Res> result{};

    for (std::size_t r = 0; r < R; ++r)
    {
        const auto lhs_row = lhs.row(r);
        for (std::size_t c = 0; c < C; ++c)
        {
            const auto rhs_col = rhs.column(c);
            result[{ r, c }] = std::inner_product(std::begin(lhs_row), std::end(lhs_row), std::begin(rhs_col), Res{});
        }
    }

    return result;
}

template <std::size_t D, class T, class U, class Res = std::invoke_result_t<std::multiplies<>, T, U>>
constexpr auto operator*(const vector_t<D, T>& lhs, const matrix_t<D + 1, D + 1, U>& rhs) -> vector_t<D, Res>
{
    vector_t<D, Res> result;

    for (std::size_t d = 0; d < D; ++d)
    {
        result[d] = std::inner_product(std::begin(lhs), std::end(lhs), std::begin(rhs.column(d)), Res{ rhs[{ D, d }] });
    }

    return result;
}

template <std::size_t D, class T, class U, class Res = std::invoke_result_t<std::multiplies<>, T, U>>
constexpr auto operator*(const matrix_t<D + 1, D + 1, T>& lhs, const vector_t<D, U>& rhs) -> vector_t<D, Res>
{
    return rhs * lhs;
}

template <std::size_t D, class T, class U, class Res = std::invoke_result_t<std::multiplies<>, T, U>>
constexpr auto operator*=(vector_t<D, T>& lhs, const matrix_t<D + 1, D + 1, U>& rhs) -> vector_t<D, T>&
{
    return lhs = lhs * rhs;
}

namespace detail
{

struct minor_fn
{
    template <std::size_t R, std::size_t C, class T>
    auto operator()(const matrix_t<R, C, T>& item, const vector_t<2, std::size_t>& loc) const -> matrix_t<R - 1, C - 1, T>
    {
        static_assert(R > 1, "minor: invalid row.");
        static_assert(C > 1, "minor: invalid col.");

        if (loc[0] >= R || loc[1] >= C)
        {
            throw std::runtime_error{ "minor: invalid row or column" };
        }

        matrix_t<R - 1, C - 1, T> result{};

        for (std::size_t r = 0; r + 1 < R; ++r)
        {
            for (std::size_t c = 0; c + 1 < C; ++c)
            {
                result[{ r, c }] = item[{ r + (r < loc[0] ? 0 : 1), c + (c < loc[1] ? 0 : 1) }];
            }
        }

        return result;
    }
};

static constexpr inline auto minor = minor_fn{};

struct transpose_fn
{
    template <std::size_t R, std::size_t C, class T>
    auto operator()(const matrix_t<R, C, T>& item) const -> matrix_t<C, R, T>
    {
        matrix_t<C, R, T> result{};

        for (std::size_t r = 0; r < R; ++r)
        {
            const auto row = item.row(r);
            const auto col = result.column(r);
            std::copy(std::begin(row), std::end(row), std::begin(col));
        }

        return result;
    }
};

static constexpr inline auto transpose = transpose_fn{};

struct determinant_fn
{
    template <class T>
    constexpr auto operator()(const matrix_t<1, 1, T>& item) const -> T
    {
        return item[{ 0, 0 }];
    }

    template <class T>
    constexpr auto operator()(const matrix_t<2, 2, T>& item) const -> T
    {
        return item[{ 0, 0 }] * item[{ 1, 1 }] - item[{ 0, 1 }] * item[{ 1, 0 }];
    }

    template <class T>
    constexpr auto operator()(const matrix_t<3, 3, T>& item) const -> T
    {
        // clang-format off
        return
            + item[{ 0, 0 }] * item[{ 1, 1 }] * item[{ 2, 2 }]
            + item[{ 0, 1 }] * item[{ 1, 2 }] * item[{ 2, 0 }]
            + item[{ 0, 2 }] * item[{ 1, 0 }] * item[{ 2, 1 }]
            - item[{ 0, 2 }] * item[{ 1, 1 }] * item[{ 2, 0 }]
            - item[{ 0, 0 }] * item[{ 1, 2 }] * item[{ 2, 1 }]
            - item[{ 0, 1 }] * item[{ 1, 0 }] * item[{ 2, 2 }];
        // clang-format on
    }

    template <std::size_t D, class T>
    constexpr auto operator()(const matrix_t<D, D, T>& item) const -> T
    {
        auto sum = T{};

        for (std::size_t i = 0; i < D; ++i)
        {
            sum += (i % 2 == 0 ? +1 : -1) * item[{ 0, i }] * (*this)(minor(item, { 0, i }));
        }

        return sum;
    }
};

static constexpr inline auto determinant = determinant_fn{};

struct invert_fn
{
    template <class T>
    static constexpr bool approx_zero(T v)
    {
        return math::abs(v) < std::numeric_limits<T>::epsilon();
    }

    template <std::size_t D, class T>
    auto operator()(const matrix_t<D, D, T>& value) const -> std::optional<matrix_t<D, D, T>>
    {
        const auto det = determinant(value);

        if (approx_zero(det))
        {
            return {};
        }

        matrix_t<D, D, T> result{};
        for (std::size_t r = 0; r < D; ++r)
        {
            for (std::size_t c = 0; c < D; ++c)
            {
                result[{ c, r }] = T((r + c) % 2 == 0 ? 1 : -1) * determinant(minor(value, { r, c })) / det;
            }
        }

        return result;
    }
};

static constexpr inline auto invert = invert_fn{};

struct identity_fn
{
    template <std::size_t D, class T = double>
    static constexpr matrix_t<D, D, T> create()
    {
        matrix_t<D, D, T> result;

        for (std::size_t r = 0; r < D; ++r)
        {
            for (std::size_t c = 0; c < D; ++c)
            {
                result[{ r, c }] = r == c ? T(1) : T(0);
            }
        }

        return result;
    }

    template <std::size_t D, class T>
    constexpr operator matrix_t<D, D, T>() const
    {
        return create<D, T>();
    }
};

static constexpr inline auto identity = identity_fn{};

struct scale_fn
{
    template <class T>
    matrix_t<3, 3, T> operator()(const vector_t<2, T>& scale) const
    {
        matrix_t<3, 3, T> result = identity;

        result[{ 0, 0 }] = scale[0];
        result[{ 1, 1 }] = scale[1];

        return result;
    }

    template <class T>
    matrix_t<4, 4, T> operator()(const vector_t<3, T>& scale) const
    {
        matrix_t<4, 4, T> result = identity;

        result[{ 0, 0 }] = scale[0];
        result[{ 1, 1 }] = scale[1];
        result[{ 2, 2 }] = scale[2];

        return result;
    }
};

struct rotation_fn
{
    template <class T>
    matrix_t<3, 3, T> operator()(T angle) const
    {
        matrix_t<3, 3, T> result = identity;

        const auto c = math::cos(angle);
        const auto s = math::sin(angle);

        result[{ 0, 0 }] = c;
        result[{ 0, 1 }] = s;
        result[{ 1, 0 }] = -s;
        result[{ 1, 1 }] = c;

        return result;
    }
};

struct translation_fn
{
    template <class T>
    matrix_t<3, 3, T> operator()(const vector_t<2, T>& offset) const
    {
        matrix_t<3, 3, T> result = identity;

        result[{ 2, 0 }] = offset[0];
        result[{ 2, 1 }] = offset[1];

        return result;
    }

    template <class T>
    matrix_t<4, 4, T> operator()(const vector_t<3, T>& offset) const
    {
        matrix_t<4, 4, T> result = identity;

        result[{ 3, 0 }] = offset[0];
        result[{ 3, 1 }] = offset[1];
        result[{ 3, 2 }] = offset[2];

        return result;
    }
};

}  // namespace detail

using detail::determinant;
using detail::invert;
using detail::minor;
using detail::transpose;

static constexpr inline auto scale = detail::scale_fn{};
static constexpr inline auto translation = detail::translation_fn{};
static constexpr inline auto rotation = detail::rotation_fn{};

}  // namespace mat

}  // namespace zx

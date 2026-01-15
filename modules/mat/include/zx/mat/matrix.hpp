#pragma once

#include <numeric>
#include <optional>
#include <zx/iterator_interface.hpp>
#include <zx/iterator_range.hpp>
#include <zx/mat/math.hpp>
#include <zx/mat/vector.hpp>

namespace zx
{

namespace mat
{

namespace detail
{
template <class T, std::ptrdiff_t N>
struct strided_iterator_impl
{
    T* m_ptr;

    explicit strided_iterator_impl(T* ptr = {}) : m_ptr{ ptr }
    {
    }

    constexpr T& deref() const
    {
        return *m_ptr;
    }

    constexpr void inc()
    {
        m_ptr += N;
    }

    constexpr void dec()
    {
        m_ptr -= N;
    }

    constexpr void advance(std::ptrdiff_t n)
    {
        m_ptr += n * N;
    }

    constexpr bool is_equal(const strided_iterator_impl& other) const
    {
        return m_ptr == other.m_ptr;
    }

    constexpr bool is_less(const strided_iterator_impl& other) const
    {
        return N > 0 ? m_ptr < other.m_ptr : m_ptr > other.m_ptr;
    }

    constexpr std::ptrdiff_t distance_to(const strided_iterator_impl& other) const
    {
        return (other.m_ptr - m_ptr) / N;
    }
};

template <class T>
struct strided_iterator_impl<T, 0>
{
    T* m_ptr;

    std::ptrdiff_t m_stride;

    strided_iterator_impl(T* ptr = {}, std::ptrdiff_t stride = 0) : m_ptr{ ptr }, m_stride{ stride }
    {
    }

    constexpr T& deref() const
    {
        return *m_ptr;
    }

    constexpr void inc()
    {
        m_ptr += m_stride;
    }

    constexpr void dec()
    {
        m_ptr -= m_stride;
    }

    constexpr void advance(std::ptrdiff_t n)
    {
        m_ptr += n * m_stride;
    }

    constexpr bool is_equal(const strided_iterator_impl& other) const
    {
        return m_ptr == other.m_ptr;
    }

    constexpr bool is_less(const strided_iterator_impl& other) const
    {
        assert(m_stride == other.m_stride);
        return m_stride > 0 ? m_ptr < other.m_ptr : m_ptr > other.m_ptr;
    }

    constexpr std::ptrdiff_t distance_to(const strided_iterator_impl& other) const
    {
        assert(m_stride == other.m_stride);
        return (other.m_ptr - m_ptr) / m_stride;
    }
};

}  // namespace detail

template <class T, std::ptrdiff_t N>
using strided_iterator = zx::iterator_interface<detail::strided_iterator_impl<T, N>>;

template <class T, std::size_t R, std::size_t C>
struct matrix_view
{
    using size_type = std::size_t;
    using volume_type = std::size_t;

    using extents_type = vector<size_type, 2>;
    using location_type = vector<size_type, 2>;

    using pointer = T*;
    using reference = T&;
    using row_type = iterator_range<pointer>;
    using column_type = iterator_range<strided_iterator<T, std::ptrdiff_t(C)>>;
    using data_type = iterator_range<pointer>;

    pointer m_data;

    constexpr matrix_view(pointer data) : m_data(data)
    {
    }

    constexpr size_type row_count() const
    {
        return R;
    }

    constexpr size_type column_count() const
    {
        return C;
    }

    constexpr extents_type extents() const
    {
        return extents_type{ row_count(), column_count() };
    }

    constexpr volume_type volume() const
    {
        return row_count() * column_count();
    }

    constexpr data_type data() const
    {
        return data_type(m_data, volume());
    }

    constexpr reference operator[](size_type n) const
    {
        return data()[n];
    }

    constexpr reference operator[](const location_type& loc) const
    {
        return *(m_data + loc[0] * column_count() + loc[1]);
    }

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

template <class T, std::size_t R, std::size_t C = R>
struct matrix
{
    using view_type = matrix_view<T, R, C>;
    using const_view_type = matrix_view<const T, R, C>;

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

    constexpr matrix() : m_data{}
    {
    }

    constexpr matrix(std::initializer_list<T> init) : matrix{}
    {
        std::copy(std::begin(init), std::end(init), std::begin(m_data));
    }

    constexpr view_type view()
    {
        return view_type{ m_data.data() };
    }

    constexpr const_view_type view() const
    {
        return const_view_type{ m_data.data() };
    }

    constexpr size_type row_count() const
    {
        return view().row_count();
    }

    constexpr size_type column_count() const
    {
        return view().column_count();
    }

    constexpr volume_type volume() const
    {
        return view().volume();
    }

    constexpr extents_type extents() const
    {
        return view().extents();
    }

    constexpr data_type data()
    {
        return view().data();
    }

    constexpr const_data_type data() const
    {
        return view().data();
    }

    constexpr reference operator[](size_type n)
    {
        return view()[n];
    }

    constexpr const_reference operator[](size_type n) const
    {
        return view()[n];
    }

    constexpr reference operator[](const location_type& loc)
    {
        return view()[loc];
    }

    constexpr const_reference operator[](const location_type& loc) const
    {
        return view()[loc];
    }

    constexpr row_type row(size_type n)
    {
        return view().row(n);
    }

    constexpr const_row_type row(size_type n) const
    {
        return view().row(n);
    }

    constexpr column_type column(size_type n)
    {
        return view().column(n);
    }

    constexpr const_column_type column(size_type n) const
    {
        return view().column(n);
    }
};

template <class T, std::size_t R, std::size_t C>
std::ostream& operator<<(std::ostream& os, const matrix_view<T, R, C>& item)
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

template <class T, std::size_t R, std::size_t C>
std::ostream& operator<<(std::ostream& os, const matrix<T, R, C>& item)
{
    return os << item.view();
}

template <class T, std::size_t R, std::size_t C>
constexpr auto operator+(const matrix<T, R, C>& item) -> matrix<T, R, C>
{
    return item;
}

template <class T, std::size_t R, std::size_t C>
constexpr auto operator-(const matrix<T, R, C>& item) -> matrix<T, R, C>
{
    matrix<T, R, C> result{};
    std::transform(std::begin(item.data()), std::end(item.data()), std::begin(result.data()), std::negate<>{});
    return result;
}

template <class T, class U, std::size_t R, std::size_t C, class = std::invoke_result_t<std::plus<>, T, U>>
constexpr auto operator+=(matrix<T, R, C>& lhs, const matrix<U, R, C>& rhs) -> matrix<T, R, C>&
{
    std::transform(
        std::begin(lhs.data()), std::end(lhs.data()), std::begin(rhs.data()), std::begin(lhs.data()), std::plus<>{});
    return lhs;
}

template <class T, class U, std::size_t R, std::size_t C, class = std::invoke_result_t<std::minus<>, T, U>>
constexpr auto operator-=(matrix<T, R, C>& lhs, const matrix<U, R, C>& rhs) -> matrix<T, R, C>&
{
    std::transform(
        std::begin(lhs.data()), std::end(lhs.data()), std::begin(rhs.data()), std::begin(lhs.data()), std::minus<>{});
    return lhs;
}

template <class T, class U, std::size_t R, std::size_t C, class = std::invoke_result_t<std::multiplies<>, T, U>>
constexpr auto operator*=(matrix<T, R, C>& lhs, U rhs) -> matrix<T, R, C>&
{
    std::transform(
        std::begin(lhs.data()),
        std::end(lhs.data()),
        std::begin(lhs.data()),
        std::bind(std::multiplies<>{}, std::placeholders::_1, rhs));
    return lhs;
}

template <class T, class U, std::size_t R, std::size_t C, class = std::invoke_result_t<std::divides<>, T, U>>
constexpr auto operator/=(matrix<T, R, C>& lhs, U rhs) -> matrix<T, R, C>&
{
    std::transform(
        std::begin(lhs.data()),
        std::end(lhs.data()),
        std::begin(lhs.data()),
        std::bind(std::divides<>{}, std::placeholders::_1, rhs));
    return lhs;
}

template <class T, class U, std::size_t R, std::size_t C, class Res = std::invoke_result_t<std::plus<>, T, U>>
constexpr auto operator+(const matrix<T, R, C>& lhs, const matrix<U, R, C>& rhs) -> matrix<Res, R, C>
{
    matrix<Res, R, C> result{};
    std::transform(
        std::begin(lhs.data()), std::end(lhs.data()), std::begin(rhs.data()), std::begin(result.data()), std::plus<>{});
    return result;
}

template <class T, class U, std::size_t R, std::size_t C, class Res = std::invoke_result_t<std::minus<>, T, U>>
constexpr auto operator-(const matrix<T, R, C>& lhs, const matrix<U, R, C>& rhs) -> matrix<Res, R, C>
{
    matrix<Res, R, C> result{};
    std::transform(
        std::begin(lhs.data()), std::end(lhs.data()), std::begin(rhs.data()), std::begin(result.data()), std::minus<>{});
    return result;
}

template <class T, class U, std::size_t R, std::size_t C, class Res = std::invoke_result_t<std::multiplies<>, T, U>>
constexpr auto operator*(const matrix<T, R, C>& lhs, U rhs) -> matrix<Res, R, C>
{
    matrix<Res, R, C> result{};
    std::transform(
        std::begin(lhs.data()),
        std::end(lhs.data()),
        std::begin(result.data()),
        std::bind(std::multiplies<>{}, std::placeholders::_1, rhs));
    return result;
}

template <class T, class U, std::size_t R, std::size_t C, class Res = std::invoke_result_t<std::multiplies<>, T, U>>
constexpr auto operator*(T lhs, const matrix<U, R, C>& rhs) -> matrix<Res, R, C>
{
    return rhs * lhs;
}

template <class T, class U, std::size_t R, std::size_t C, class Res = std::invoke_result_t<std::divides<>, T, U>>
constexpr auto operator/(const matrix<T, R, C>& lhs, U rhs) -> matrix<Res, R, C>
{
    matrix<Res, R, C> result{};
    std::transform(
        std::begin(lhs.data()),
        std::end(lhs.data()),
        std::begin(result.data()),
        std::bind(std::divides<>{}, std::placeholders::_1, rhs));
    return result;
}

template <class T, class U, std::size_t R, std::size_t C, class = std::invoke_result_t<std::equal_to<>, T, U>>
constexpr bool operator==(const matrix<T, R, C>& lhs, const matrix<U, R, C>& rhs)
{
    return std::equal(std::begin(lhs.data()), std::end(lhs.data()), std::begin(rhs.data()));
}

template <class T, class U, std::size_t R, std::size_t C, class = std::invoke_result_t<std::equal_to<>, T, U>>
constexpr bool operator!=(const matrix<T, R, C>& lhs, const matrix<U, R, C>& rhs)
{
    return !(lhs == rhs);
}

template <
    class T,
    class U,
    std::size_t R,
    std::size_t D,
    std::size_t C,
    class Res = std::invoke_result_t<std::multiplies<>, T, U>>
constexpr auto operator*(const matrix<T, R, D>& lhs, const matrix<U, D, C>& rhs) -> matrix<Res, R, C>
{
    matrix<Res, R, C> result{};

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

template <class T, class U, std::size_t D, class Res = std::invoke_result_t<std::multiplies<>, T, U>>
constexpr auto operator*(const vector<T, D>& lhs, const matrix<U, D + 1>& rhs) -> vector<Res, D>
{
    vector<Res, D> result;

    for (std::size_t d = 0; d < D; ++d)
    {
        result[d] = std::inner_product(std::begin(lhs), std::end(lhs), std::begin(rhs.column(d)), Res{ rhs[{ D, d }] });
    }

    return result;
}

template <class T, class U, std::size_t D, class Res = std::invoke_result_t<std::multiplies<>, T, U>>
constexpr auto operator*(const matrix<T, D + 1>& lhs, const vector<U, D>& rhs) -> vector<Res, D>
{
    return rhs * lhs;
}

template <class T, class U, std::size_t D, class = std::invoke_result_t<std::multiplies<>, T, U>>
constexpr auto operator*=(vector<T, D>& lhs, const matrix<U, D + 1>& rhs) -> vector<T, D>&
{
    return lhs = lhs * rhs;
}

namespace detail
{

struct minor_fn
{
    template <class T, std::size_t R, std::size_t C>
    auto operator()(const matrix<T, R, C>& item, const vector<std::size_t, 2>& loc) const -> matrix<T, R - 1, C - 1>
    {
        static_assert(R > 1, "minor: invalid row.");
        static_assert(C > 1, "minor: invalid col.");

        if (loc[0] >= R || loc[1] >= C)
        {
            throw std::runtime_error{ "minor: invalid row or column" };
        }

        matrix<T, R - 1, C - 1> result{};

        for (std::size_t r = 0; r < R; ++r)
        {
            for (std::size_t c = 0; c < C; ++c)
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
    template <class T, std::size_t R, std::size_t C>
    auto operator()(const matrix<T, R, C>& item) const -> matrix<T, C, R>
    {
        matrix<T, C, R> result{};

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
    constexpr auto operator()(const matrix<T, 1>& item) const -> T
    {
        return item[{ 0, 0 }];
    }

    template <class T>
    constexpr auto operator()(const matrix<T, 2>& item) const -> decltype(std::declval<T>() * std::declval<T>())
    {
        return item[{ 0, 0 }] * item[{ 1, 1 }] - item[{ 0, 1 }] * item[{ 1, 0 }];
    }

    template <class T>
    constexpr auto operator()(const matrix<T, 3>& item) const
        -> decltype(std::declval<T>() * std::declval<T>() * std::declval<T>())
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

    template <class T, std::size_t D>
    constexpr auto operator()(const matrix<T, D>& item) const
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
        return math::abs(v) < T(1e-10);
    }

    template <class T, std::size_t D>
    auto operator()(const matrix<T, D>& value) const -> std::optional<matrix<T, D>>
    {
        const auto det = determinant(value);

        if (approx_zero(det))
        {
            return {};
        }

        matrix<T, D> result{};
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
    template <size_t D, class T = double>
    static constexpr matrix<T, D> create()
    {
        matrix<T, D> result;

        for (std::size_t r = 0; r < D; ++r)
        {
            for (std::size_t c = 0; c < D; ++c)
            {
                result[{ r, c }] = r == c ? T(1) : T(0);
            }
        }

        return result;
    }

    template <class T, std::size_t D>
    constexpr operator matrix<T, D>() const
    {
        return create<D, T>();
    }
};

static constexpr inline auto identity = identity_fn{};

struct scale_fn
{
    template <class T>
    matrix<T, 3> operator()(const vector<T, 2>& scale) const
    {
        matrix<T, 3> result = identity;

        result[{ 0, 0 }] = scale[0];
        result[{ 1, 1 }] = scale[1];

        return result;
    }

    template <class T>
    matrix<T, 4> operator()(const vector<T, 3>& scale) const
    {
        matrix<T, 4> result = identity;

        result[{ 0, 0 }] = scale[0];
        result[{ 1, 1 }] = scale[1];
        result[{ 2, 2 }] = scale[2];

        return result;
    }
};

struct rotation_fn
{
    template <class T>
    matrix<T, 3> operator()(T angle) const
    {
        matrix<T, 3> result = identity;

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
    matrix<T, 3> operator()(const vector<T, 2>& offset) const
    {
        matrix<T, 3> result = identity;

        result[{ 2, 0 }] = offset[0];
        result[{ 2, 1 }] = offset[1];

        return result;
    }

    template <class T>
    matrix<T, 4> operator()(const vector<T, 3>& offset) const
    {
        matrix<T, 4> result = identity;

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

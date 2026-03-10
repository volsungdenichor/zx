#pragma once

#include <algorithm>
#include <array>
#include <numeric>
#include <optional>
#include <sstream>
#include <tuple>
#include <zx/iterator_interface.hpp>
#include <zx/mat.hpp>

namespace zx
{

namespace arrays
{

using location_base_t = std::ptrdiff_t;
using size_base_t = location_base_t;
using stride_base_t = location_base_t;

using flat_offset_t = std::ptrdiff_t;
using volume_t = std::ptrdiff_t;

struct slice_base_t
{
    std::optional<location_base_t> start = {};
    std::optional<location_base_t> stop = {};
    std::optional<stride_base_t> step = {};

    friend bool operator==(const slice_base_t& lhs, const slice_base_t& rhs)
    {
        return std::tie(lhs.start, lhs.stop, lhs.step) == std::tie(rhs.start, rhs.stop, rhs.step);
    }

    friend bool operator!=(const slice_base_t& lhs, const slice_base_t& rhs) { return !(lhs == rhs); }

    friend std::ostream& operator<<(std::ostream& os, const slice_base_t& item)
    {
        if (item.start)
        {
            os << *item.start;
        }
        os << ":";
        if (item.stop)
        {
            os << *item.stop;
        }
        os << ":";
        if (item.step)
        {
            os << *item.step;
        }
        return os;
    }
};

struct dim_t
{
    size_base_t size = 0;
    stride_base_t stride = 1;

    friend bool operator==(const dim_t& lhs, const dim_t& rhs)
    {
        return std::tie(lhs.size, lhs.stride) == std::tie(rhs.size, rhs.stride);
    }

    friend bool operator!=(const dim_t& lhs, const dim_t& rhs) { return !(lhs == rhs); }

    friend std::ostream& operator<<(std::ostream& os, const dim_t& item)
    {
        return os << "{"
                  << ":size " << item.size << " "
                  << ":stride " << item.stride << "}";
    }

    location_base_t adjust_location(location_base_t loc) const { return loc >= 0 ? loc : (loc + size); }
    mat::interval_t<location_base_t> bounds() const { return { 0, size }; }

    flat_offset_t flat_offset(const location_base_t& loc) const { return loc * stride; }

    std::pair<dim_t, location_base_t> slice(const slice_base_t& s) const
    {
        const auto clamp = [&](location_base_t value, location_base_t init) -> location_base_t
        { return std::max(init, std::min(value, this->size + init)); };

        const stride_base_t actual_step = s.step.value_or(1);
        location_base_t actual_start, actual_stop;

        if (actual_step > 0)
        {
            actual_start = s.start.value_or(0);
            actual_stop = s.stop.value_or(this->size);

            actual_start = clamp(actual_start + (actual_start < 0 ? this->size : 0), 0);
            actual_stop = clamp(actual_stop + (actual_stop < 0 ? this->size : 0), 0);
        }
        else
        {
            actual_start = s.start.value_or(this->size - 1);
            actual_stop = s.stop.value_or(-this->size - 1);

            actual_start = clamp(actual_start + (actual_start < 0 ? this->size : 0), -1);
            actual_stop = clamp(actual_stop + (actual_stop < 0 ? this->size : 0), -1);
        }

        const size_base_t new_size
            = actual_step > 0
                  ? std::max(location_base_t(0), (actual_stop - actual_start + actual_step - 1) / actual_step)
                  : std::max(location_base_t(0), (actual_start - actual_stop - actual_step - 1) / (-actual_step));

        const stride_base_t new_stride = this->stride * actual_step;
        const location_base_t new_start = actual_start * this->stride;

        return { dim_t{ new_size, new_stride }, new_start };
    }
};

template <std::size_t D>
struct shape_t
{
    using size_type = zx::mat::vector_t<size_base_t, D>;
    using stride_type = zx::mat::vector_t<stride_base_t, D>;
    using location_type = zx::mat::vector_t<location_base_t, D>;
    using slice_type = zx::mat::vector_t<slice_base_t, D>;
    using bounds_type = zx::mat::box_shape_t<size_base_t, D>;

    using dims_type = std::array<dim_t, D>;

    dims_type m_dims = {};

    shape_t() = default;

    shape_t(dims_type dims) : m_dims(dims) { }

    template <class... Tail>
    shape_t(dim_t head, Tail... tail) : m_dims{ head, static_cast<dim_t>(tail)... }
    {
        static_assert(sizeof...(tail) + 1 == D, "Invalid number of arguments to shape_t constructor");
    }

    const dim_t& dim(std::size_t d) const { return m_dims[d]; }

    friend bool operator==(const shape_t& lhs, const shape_t& rhs) { return lhs.m_dims == rhs.m_dims; }
    friend bool operator!=(const shape_t& lhs, const shape_t& rhs) { return !(lhs == rhs); }

    friend std::ostream& operator<<(std::ostream& os, const shape_t& item)
    {
        return os << "{ :size " << item.size() << " :stride " << item.stride() << " }";
    }

    volume_t volume() const
    {
        volume_t result = 1;
        for (std::size_t d = 0; d < D; ++d)
        {
            result *= m_dims[d].size;
        }
        return result;
    }

    size_type size() const
    {
        size_type result = {};
        for (std::size_t d = 0; d < D; ++d)
        {
            result[d] = m_dims[d].size;
        }
        return result;
    }

    stride_type stride() const
    {
        stride_type result = {};
        for (std::size_t d = 0; d < D; ++d)
        {
            result[d] = m_dims[d].stride;
        }
        return result;
    }

    bounds_type bounds() const
    {
        bounds_type result = {};
        for (std::size_t d = 0; d < D; ++d)
        {
            result[d] = m_dims[d].bounds();
        }
        return result;
    }

    location_type adjust_location(const location_type& loc) const
    {
        location_type result = {};
        for (std::size_t d = 0; d < D; ++d)
        {
            result[d] = m_dims[d].adjust_location(loc[d]);
        }
        return result;
    }

    flat_offset_t flat_offset(const location_type& loc) const
    {
        flat_offset_t result = 0;
        for (std::size_t d = 0; d < D; ++d)
        {
            result += m_dims[d].flat_offset(loc[d]);
        }
        return result;
    }

    static shape_t from_size(const size_type& size)
    {
        shape_t result;
        stride_base_t stride = 1;
        for (int _d = D - 1; _d >= 0; --_d)
        {
            const auto d = static_cast<std::size_t>(_d);
            auto& dim = result.m_dims[d];
            dim.size = size[d];
            dim.stride = stride;
            stride *= dim.size;
        }
        return result;
    }

    std::pair<shape_t, location_type> slice(const slice_type& s) const
    {
        shape_t new_shape = *this;
        location_type new_start = {};
        for (std::size_t d = 0; d < D; ++d)
        {
            std::tie(new_shape.m_dims[d], new_start[d]) = m_dims[d].slice(s[d]);
        }
        return { new_shape, new_start };
    }

    std::pair<shape_t, location_type> region(const bounds_type& b) const
    {
        slice_type s = {};
        for (std::size_t d = 0; d < D; ++d)
        {
            s[d].start = mat::lower(b[d]);
            s[d].stop = mat::upper(b[d]);
        }
        return slice(s);
    }

    template <std::size_t D_ = D, std::enable_if_t<(D_ > 1), int> = 0>
    shape_t<D - 1> erase(std::size_t index) const
    {
        typename shape_t<D - 1>::dims_type result = {};
        for (std::size_t i = 0, j = 0; i < D; ++i)
        {
            if (i != index)
            {
                result[j++] = m_dims[i];
            }
        }
        return shape_t<D - 1>{ result };
    }
};

template <>
struct shape_t<1>
{
    using size_type = size_base_t;
    using stride_type = stride_base_t;
    using location_type = location_base_t;
    using slice_type = slice_base_t;
    using bounds_type = zx::mat::interval_t<size_base_t>;

    using dims_type = std::array<dim_t, 1>;

    dims_type m_dims = {};

    shape_t() = default;

    shape_t(dims_type dims) : m_dims(dims) { }

    shape_t(dim_t head) : m_dims{ std::move(head) } { }

    const dim_t& dim(std::size_t d) const { return m_dims[d]; }

    friend bool operator==(const shape_t& lhs, const shape_t& rhs) { return lhs.m_dims == rhs.m_dims; }
    friend bool operator!=(const shape_t& lhs, const shape_t& rhs) { return !(lhs == rhs); }

    friend std::ostream& operator<<(std::ostream& os, const shape_t& item)
    {
        return os << "{ :size " << item.size() << " :stride " << item.stride() << " }";
    }

    volume_t volume() const { return m_dims[0].size; }

    size_type size() const { return m_dims[0].size; }

    stride_type stride() const { return m_dims[0].stride; }

    bounds_type bounds() const { return m_dims[0].bounds(); }

    location_type adjust_location(const location_type& loc) const { return m_dims[0].adjust_location(loc); }

    flat_offset_t flat_offset(const location_type& loc) const { return m_dims[0].flat_offset(loc); }

    static shape_t from_size(const size_type& size) { return shape_t{ dim_t{ size, 1 } }; }

    std::pair<shape_t, location_type> slice(const slice_type& s) const { return m_dims[0].slice(s); }

    std::pair<shape_t, location_type> region(const bounds_type& b) const
    {
        return m_dims[0].slice(slice_type{ { mat::lower(b) }, { mat::upper(b) } });
    }
};

template <class T, std::size_t D>
struct array_view_t
{
    using value_type = std::remove_const_t<T>;
    using shape_type = shape_t<D>;
    using pointer = T*;
    using reference = T&;

    using iterator = void;

    using location_type = typename shape_type::location_type;
    using size_type = typename shape_type::size_type;
    using stride_type = typename shape_type::stride_type;
    using slice_type = typename shape_type::slice_type;
    using bounds_type = typename shape_type::bounds_type;

    array_view_t(pointer data, shape_type shape) : m_data{ data }, m_shape{ shape } { }
    array_view_t(const array_view_t&) = default;
    array_view_t(array_view_t&&) noexcept = default;

    array_view_t& operator=(const array_view_t&) = default;
    array_view_t& operator=(array_view_t&&) noexcept = default;

    const shape_type& shape() const { return m_shape; }

    array_view_t<std::add_const_t<T>, D> as_const() const { return { m_data, m_shape }; }

    operator array_view_t<std::add_const_t<T>, D>() const { return as_const(); }

    size_type size() const { return m_shape.size(); }
    stride_type stride() const { return m_shape.stride(); }
    location_type start() const { return m_shape.start(); }
    volume_t volume() const { return m_shape.volume(); }
    bounds_type bounds() const { return m_shape.bounds(); }

    pointer data() const { return m_data + m_shape.flat_offset(location_type{}); }

    pointer get(const location_type& loc) const
    {
        const location_type adjusted_loc = m_shape.adjust_location(loc);
        if (!mat::contains(bounds(), adjusted_loc))
        {
            throw std::out_of_range{
                (std::ostringstream() << "Location " << loc << " is out of bounds (" << size() << ")").str()
            };
        }
        return m_data + m_shape.flat_offset(adjusted_loc);
    }

    reference operator[](const location_type& loc) const { return *get(loc); }

    array_view_t slice(const slice_type& s) const
    {
        const auto [new_shape, new_start] = m_shape.slice(s);
        const flat_offset_t offset = std::accumulate(new_start.begin(), new_start.end(), flat_offset_t{ 0 });
        return array_view_t{ m_data + offset, new_shape };
    }

    array_view_t region(const bounds_type& b) const
    {
        slice_type s = {};
        for (std::size_t d = 0; d < D; ++d)
        {
            s[d].start = mat::lower(b[d]);
            s[d].stop = mat::upper(b[d]);
        }
        return slice(s);
    }

    array_view_t<T, D - 1> sub(std::size_t d, location_base_t n) const
    {
        const location_base_t adjusted_loc = m_shape.dim(d).adjust_location(n);
        if (!mat::contains(m_shape.dim(d).bounds(), adjusted_loc))
        {
            throw std::out_of_range{
                (std::ostringstream() << "Index " << n << " is out of bounds (" << m_shape.dim(d).size << ")").str()
            };
        }
        const auto offset = adjusted_loc * m_shape.dim(d).stride;
        return array_view_t<T, D - 1>{ m_data + offset, m_shape.erase(d) };
    }

    array_view_t<T, D - 1> operator[](location_base_t n) const { return sub(0, n); }

    template <class T_ = T, std::enable_if_t<!std::is_const_v<T_>, int> = 0>
    void fill(const value_type& value)
    {
        for (size_base_t i = 0; i < m_shape.dim(0).size; ++i)
        {
            (*this)[i].fill(value);
        }
    }

    friend std::ostream& operator<<(std::ostream& os, const array_view_t& item) { return os << item.shape(); }

    pointer m_data;
    shape_type m_shape;
};

template <class T>
struct array_view_t<T, 1>
{
    using value_type = std::remove_const_t<T>;
    using shape_type = shape_t<1>;
    using pointer = T*;
    using reference = T&;

    using iterator = strided_iterator<pointer>;

    using location_type = typename shape_type::location_type;
    using size_type = typename shape_type::size_type;
    using stride_type = typename shape_type::stride_type;
    using slice_type = typename shape_type::slice_type;
    using bounds_type = typename shape_type::bounds_type;

    array_view_t(pointer data, shape_type shape) : m_data{ data }, m_shape{ shape } { }
    array_view_t(const array_view_t&) = default;
    array_view_t(array_view_t&&) noexcept = default;

    array_view_t& operator=(const array_view_t&) = default;
    array_view_t& operator=(array_view_t&&) noexcept = default;

    const shape_type& shape() const { return m_shape; }

    array_view_t<std::add_const_t<T>, 1> as_const() const { return { m_data, m_shape }; }

    operator array_view_t<std::add_const_t<T>, 1>() const { return as_const(); }

    size_type size() const { return m_shape.size(); }
    stride_type stride() const { return m_shape.stride(); }
    volume_t volume() const { return m_shape.volume(); }
    bounds_type bounds() const { return m_shape.bounds(); }

    pointer data() const { return m_data + m_shape.flat_offset(location_type{}); }

    pointer get(location_type loc) const
    {
        const location_type adjusted_loc = m_shape.adjust_location(loc);
        if (!mat::contains(bounds(), adjusted_loc))
        {
            throw std::out_of_range{
                (std::ostringstream() << "Location " << loc << " is out of bounds (" << size() << ")").str()
            };
        }
        return m_data + m_shape.flat_offset(adjusted_loc);
    }

    reference operator[](location_type loc) const { return *get(loc); }

    array_view_t slice(const slice_type& s) const
    {
        const auto [new_shape, new_start] = m_shape.slice(s);
        return array_view_t{ m_data + new_start, new_shape };
    }

    array_view_t region(const bounds_type& b) const { return slice(slice_type{ { mat::lower(b) }, { mat::upper(b) } }); }

    iterator begin() const { return iterator{ m_data, stride() }; }
    iterator end() const { return begin() + volume(); }

    template <class T_ = T, std::enable_if_t<!std::is_const_v<T_>, int> = 0>
    void fill(const value_type& value)
    {
        std::fill(begin(), end(), value);
    }

    friend std::ostream& operator<<(std::ostream& os, const array_view_t& item) { return os << item.shape(); }

    pointer m_data;
    shape_type m_shape;
};

template <class T, std::size_t D>
struct array_t
{
    using value_type = T;
    using mut_view_type = array_view_t<T, D>;
    using view_type = array_view_t<const T, D>;

    template <std::size_t D_>
    using mut_sub_view_type = array_view_t<T, D_>;

    template <std::size_t D_>
    using sub_view_type = array_view_t<const T, D_>;

    using shape_type = typename view_type::shape_type;
    using location_type = typename view_type::location_type;
    using size_type = typename view_type::size_type;
    using stride_type = typename view_type::stride_type;
    using bounds_type = typename view_type::bounds_type;
    using slice_type = typename view_type::slice_type;

    using const_pointer = typename view_type::pointer;
    using const_reference = typename view_type::reference;
    using const_iterator = typename view_type::iterator;

    using pointer = typename mut_view_type::pointer;
    using reference = typename mut_view_type::reference;
    using iterator = typename mut_view_type::iterator;

    array_t(const size_type size, const T& init = {})
        : m_shape{ shape_type::from_size(size) }
        , m_data(static_cast<std::size_t>(m_shape.volume()), init)
    {
    }

    template <std::size_t D_ = D, std::enable_if_t<(D_ == 1), int> = 0>
    explicit array_t(std::vector<T> init)
        : m_shape{ shape_type::from_size(size_type{ static_cast<size_base_t>(init.size()) }) }
        , m_data(std::move(init))
    {
    }

    array_t(const array_t&) = default;
    array_t(array_t&&) noexcept = default;
    array_t& operator=(const array_t&) = default;
    array_t& operator=(array_t&&) noexcept = default;

    const shape_type& shape() const { return m_shape; }

    view_type view() const { return { m_data.data(), m_shape }; }
    mut_view_type mut_view() { return { m_data.data(), m_shape }; }

    operator view_type() const { return view(); }
    operator mut_view_type() { return mut_view(); }

    size_type size() const { return view().size(); }
    stride_type stride() const { return view().stride(); }
    volume_t volume() const { return view().volume(); }
    bounds_type bounds() const { return view().bounds(); }

    const_reference operator[](const location_type& loc) const { return view()[loc]; }
    reference operator[](const location_type& loc) { return mut_view()[loc]; }

    template <std::size_t D_ = D, std::enable_if_t<(D_ > 1), int> = 0>
    sub_view_type<D_ - 1> operator[](location_base_t n) const
    {
        return view()[n];
    }

    template <std::size_t D_ = D, std::enable_if_t<(D_ > 1), int> = 0>
    mut_sub_view_type<D_ - 1> operator[](location_base_t n)
    {
        return mut_view()[n];
    }

    mut_view_type slice(const slice_type& s) { return mut_view().slice(s); }
    view_type slice(const slice_type& s) const { return view().slice(s); }

    mut_view_type region(const bounds_type& b) { return mut_view().region(b); }
    view_type region(const bounds_type& b) const { return view().region(b); }

    iterator begin() { return mut_view().begin(); }
    iterator end() { return mut_view().end(); }

    const_iterator begin() const { return view().begin(); }
    const_iterator end() const { return view().end(); }

    friend std::ostream& operator<<(std::ostream& os, const array_t& item) { return os << item.shape(); }

    shape_type m_shape;
    std::vector<T> m_data;
};

struct adjust_bounds_fn
{
    inline auto operator()(mat::interval_t<size_base_t> dst, mat::interval_t<size_base_t> src, location_base_t location)
        const -> std::pair<mat::interval_t<size_base_t>, mat::interval_t<size_base_t>>
    {
        constexpr auto adjust
            = [](mat::interval_t<size_base_t> interval, size_base_t lo, size_base_t up) -> mat::interval_t<size_base_t>
        {
            lo = std::clamp(lo, mat::lower(interval), mat::upper(interval));
            up = std::clamp(up, mat::lower(interval), mat::upper(interval));
            up = std::max(up, lo);
            return mat::interval_t<size_base_t>{ lo, up };
        };
        const auto new_dst = adjust(
            dst,
            std::max(mat::lower(dst), mat::lower(src) + location),
            std::min(mat::upper(dst), mat::upper(src) + location));

        const auto new_src = adjust(src, mat::lower(new_dst) - location, mat::upper(new_dst) - location);

        return { new_src, new_dst };
    }

    template <std::size_t D>
    auto operator()(
        mat::box_shape_t<size_base_t, D> dst,
        mat::box_shape_t<size_base_t, D> src,
        const mat::vector_t<location_base_t, D>& location) const
        -> std::pair<mat::box_shape_t<size_base_t, D>, mat::box_shape_t<size_base_t, D>>
    {
        mat::box_shape_t<size_base_t, D> new_dst = dst;
        mat::box_shape_t<size_base_t, D> new_src = src;

        for (std::size_t d = 0; d < D; ++d)
        {
            std::tie(new_src[d], new_dst[d]) = (*this)(dst[d], src[d], location[d]);
        }

        return { new_src, new_dst };
    }
};

static constexpr inline auto adjust_bounds = adjust_bounds_fn{};

struct copy_fn
{
    template <class T, class U>
    void operator()(array_view_t<T, 1> dst, array_view_t<U, 1> src) const
    {
        if (dst.size() != src.size())
        {
            throw std::invalid_argument{ "Source and destination sizes do not match" };
        }

        for (size_base_t i = 0; i < dst.size(); ++i)
        {
            dst[i] = src[i];
        }
    }

    template <class T, class U, std::size_t D>
    void operator()(array_view_t<T, D> dst, array_view_t<U, D> src) const
    {
        if (dst.size() != src.size())
        {
            throw std::invalid_argument{ "Source and destination sizes do not match" };
        }

        for (size_base_t i = 0; i < dst.shape().dim(0).size; ++i)
        {
            (*this)(dst[i], src[i]);
        }
    }

    template <class T, class U, std::size_t D>
    void operator()(
        array_view_t<T, D> dst, array_view_t<U, D> src, const typename array_view_t<T, D>::location_type& location) const
    {
        const auto [src_bounds, dst_bounds] = adjust_bounds(dst.bounds(), src.bounds(), location);
        (*this)(dst.region(dst_bounds), src.region(src_bounds));
    }
};

static constexpr inline auto copy = copy_fn{};

}  // namespace arrays

}  // namespace zx

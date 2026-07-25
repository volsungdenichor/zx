#pragma once

#include <algorithm>
#include <array>
#include <numeric>
#include <optional>
#include <sstream>
#include <tuple>
#include <zx/algorithm.hpp>
#include <zx/format.hpp>
#include <zx/iterator_interface.hpp>
#include <zx/mat.hpp>

namespace zx
{

namespace mat
{

using byte_ptr = std::uint8_t*;

using location_base_t = int;
using extent_base_t = location_base_t;
using stride_base_t = location_base_t;
using interval_type = interval_t<extent_base_t>;

using flat_offset_t = std::ptrdiff_t;
using volume_t = std::ptrdiff_t;

template <class T>
byte_ptr to_byte_ptr(T* ptr)
{
    return const_cast<std::uint8_t*>(reinterpret_cast<const std::uint8_t*>(ptr));
}

template <class T>
T to_ptr(byte_ptr ptr, flat_offset_t value)
{
    return reinterpret_cast<T>(ptr + value);
}

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
    extent_base_t extent = 0;
    stride_base_t stride = 1;

    friend bool operator==(const dim_t& lhs, const dim_t& rhs)
    {
        return std::tie(lhs.extent, lhs.stride) == std::tie(rhs.extent, rhs.stride);
    }

    friend bool operator!=(const dim_t& lhs, const dim_t& rhs) { return !(lhs == rhs); }

    friend std::ostream& operator<<(std::ostream& os, const dim_t& item)
    {
        return os << "{"
                  << ":extent " << item.extent << " "
                  << ":stride " << item.stride << "}";
    }

    location_base_t adjust_location(location_base_t loc) const { return loc >= 0 ? loc : (loc + extent); }
    interval_t<location_base_t> bounds() const { return { 0, extent }; }

    flat_offset_t flat_offset(const location_base_t& loc) const { return loc * stride; }

    dim_t flip() const { return { extent, -stride }; }

    std::pair<dim_t, location_base_t> slice(const slice_base_t& s) const
    {
        const auto clamp = [&](location_base_t value, location_base_t init) -> location_base_t
        { return std::max(init, std::min(value, this->extent + init)); };

        const stride_base_t actual_step = s.step.value_or(1);
        location_base_t actual_start, actual_stop;

        if (actual_step > 0)
        {
            actual_start = s.start.value_or(0);
            actual_stop = s.stop.value_or(this->extent);

            actual_start = clamp(actual_start + (actual_start < 0 ? this->extent : 0), 0);
            actual_stop = clamp(actual_stop + (actual_stop < 0 ? this->extent : 0), 0);
        }
        else
        {
            actual_start = s.start.value_or(this->extent - 1);
            actual_stop = s.stop.value_or(-this->extent - 1);

            actual_start = clamp(actual_start + (actual_start < 0 ? this->extent : 0), -1);
            actual_stop = clamp(actual_stop + (actual_stop < 0 ? this->extent : 0), -1);
        }

        const extent_base_t new_size
            = actual_step > 0
                  ? std::max(location_base_t(0), (actual_stop - actual_start + actual_step - 1) / actual_step)
                  : std::max(location_base_t(0), (actual_start - actual_stop - actual_step - 1) / (-actual_step));

        const stride_base_t new_stride = this->stride * actual_step;
        const location_base_t new_start = actual_start * this->stride;

        return { dim_t{ new_size, new_stride }, new_start };
    }
};

template <std::size_t D>
using stride_t = vector_t<D, stride_base_t>;

template <std::size_t D>
using location_t = point_t<D, location_base_t>;

template <std::size_t D>
using slice_t = vector_t<D, slice_base_t>;

template <std::size_t D>
using bounds_t = box_shape_t<D, extent_base_t>;

template <std::size_t D, class...>
struct shape_t : md_base_t<D, dim_t, shape_t>
{
    using extent_type = extent_t<D, extent_base_t>;
    using stride_type = stride_t<D>;
    using location_type = location_t<D>;
    using slice_type = slice_t<D>;
    using bounds_type = bounds_t<D>;

    using dims_type = std::array<dim_t, D>;

    using base_t = md_base_t<D, dim_t, shape_t>;
    using base_t::base_t;

    const dim_t& dim(std::size_t d) const { return (*this)[d]; }

    friend std::ostream& operator<<(std::ostream& os, const shape_t& item)
    {
        return os << "{ :extent " << item.extent() << " :stride " << item.stride() << " }";
    }

    volume_t volume() const
    {
        volume_t result = 1;
        for (std::size_t d = 0; d < D; ++d)
        {
            result *= (*this)[d].extent;
        }
        return result;
    }

    extent_type extent() const { return detail::map_into(extent_type{}, &dim_t::extent, *this); }

    stride_type stride() const { return detail::map_into(stride_type{}, &dim_t::stride, *this); }

    bounds_type bounds() const { return detail::map_into(bounds_type{}, &dim_t::bounds, *this); }

    location_type adjust_location(const location_type& loc) const
    {
        return detail::map_into(
            location_type{},
            [&](const dim_t& dim, const location_base_t& l) -> location_base_t { return dim.adjust_location(l); },
            *this,
            loc);
    }

    flat_offset_t flat_offset(const location_type& loc) const
    {
        flat_offset_t result = 0;
        for (std::size_t d = 0; d < D; ++d)
        {
            result += dim(d).flat_offset(loc[d]);
        }
        return result;
    }

    static shape_t from_extent(const extent_type& extent, extent_base_t element_size)
    {
        shape_t result;
        stride_base_t stride = element_size;
        for (int _d = D - 1; _d >= 0; --_d)
        {
            const auto d = static_cast<std::size_t>(_d);
            auto& dim = result[d];
            dim.extent = extent[d];
            dim.stride = stride;
            stride *= dim.extent;
        }
        return result;
    }

    std::pair<shape_t, location_type> slice(const slice_type& s) const
    {
        shape_t new_shape = *this;
        location_type new_start = {};
        for (std::size_t d = 0; d < D; ++d)
        {
            std::tie(new_shape[d], new_start[d]) = dim(d).slice(s[d]);
        }
        return { new_shape, new_start };
    }

    template <std::size_t D_ = D, enable_if_t<(D_ > 1)> = 0>
    shape_t<D - 1> erase(std::size_t index) const
    {
        typename shape_t<D - 1>::dims_type result = {};
        for (std::size_t i = 0, j = 0; i < D; ++i)
        {
            if (i != index)
            {
                result[j++] = dim(i);
            }
        }
        return shape_t<D - 1>{ result };
    }
};

template <>
struct shape_t<1>
{
    using extent_type = extent_base_t;
    using stride_type = stride_base_t;
    using location_type = location_base_t;
    using slice_type = slice_base_t;
    using bounds_type = interval_type;

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
        return os << "{ :extent " << item.extent() << " :stride " << item.stride() << " }";
    }

    volume_t volume() const { return m_dims[0].extent; }

    extent_type extent() const { return m_dims[0].extent; }

    stride_type stride() const { return m_dims[0].stride; }

    bounds_type bounds() const { return m_dims[0].bounds(); }

    location_type adjust_location(const location_type& loc) const { return m_dims[0].adjust_location(loc); }

    flat_offset_t flat_offset(const location_type& loc) const { return m_dims[0].flat_offset(loc); }

    static shape_t from_extent(const extent_type& extent, extent_base_t element_size)
    {
        return shape_t{ dim_t{ extent, element_size } };
    }

    std::pair<shape_t, location_type> slice(const slice_type& s) const
    {
        auto [new_dim, new_start] = m_dims[0].slice(s);
        return { shape_t{ new_dim }, new_start };
    }
};

namespace detail
{

template <class T, std::size_t D>
struct flat_iter_impl
{
    flat_iter_impl(T* data = {}, shape_t<D> shape = {}, volume_t index = 0)
        : m_data{ to_byte_ptr(data) }
        , m_shape{ std::move(shape) }
        , m_index{ index }
    {
        volume_t pitch = 1;
        for (std::size_t rd = 0; rd < D; ++rd)
        {
            const auto d = D - 1 - rd;
            m_pitch[d] = pitch;
            pitch *= m_shape.dim(d).extent;
        }
    }

    T& deref() const
    {
        flat_offset_t offset = 0;
        volume_t idx = m_index;
        for (std::size_t d = 0; d < D; ++d)
        {
            const auto loc = static_cast<location_base_t>((idx / m_pitch[d]) % m_shape.dim(d).extent);
            offset += m_shape.dim(d).flat_offset(loc);
        }
        return *to_ptr<T*>(m_data, offset);
    }

    void inc() { ++m_index; }

    void dec() { --m_index; }

    void advance(std::ptrdiff_t n) { m_index += n; }

    bool is_equal(const flat_iter_impl& other) const { return m_data == other.m_data && m_index == other.m_index; }

    bool is_less(const flat_iter_impl& other) const
    {
        assert(m_data == other.m_data);
        return m_index < other.m_index;
    }

    std::ptrdiff_t distance_to(const flat_iter_impl& other) const
    {
        assert(m_data == other.m_data);
        return other.m_index - m_index;
    }

    byte_ptr m_data;
    shape_t<D> m_shape;
    std::array<volume_t, D> m_pitch = {};
    volume_t m_index;
};

template <class T>
struct flat_iter_impl<T, 1>
{
    flat_iter_impl(T* data = {}, shape_t<1> shape = {}, volume_t index = 0)
        : m_data{ to_byte_ptr(data) }
        , m_shape{ std::move(shape) }
        , m_index{ index }
    {
    }

    T& deref() const
    {
        const auto loc = static_cast<location_base_t>(m_index);
        return *to_ptr<T*>(m_data, m_shape.flat_offset(loc));
    }

    void inc() { ++m_index; }

    void dec() { --m_index; }

    void advance(std::ptrdiff_t n) { m_index += n; }

    bool is_equal(const flat_iter_impl& other) const { return m_data == other.m_data && m_index == other.m_index; }

    bool is_less(const flat_iter_impl& other) const
    {
        assert(m_data == other.m_data);
        return m_index < other.m_index;
    }

    std::ptrdiff_t distance_to(const flat_iter_impl& other) const
    {
        assert(m_data == other.m_data);
        return other.m_index - m_index;
    }

    byte_ptr m_data;
    shape_t<1> m_shape;
    volume_t m_index;
};

}  // namespace detail

template <class T, std::size_t D>
struct array_view_base_t
{
    using value_type = std::remove_const_t<T>;
    using shape_type = shape_t<D>;
    using pointer = T*;
    using reference = T&;

    using iterator = iterator_interface<detail::flat_iter_impl<T, D>>;

    using location_type = typename shape_type::location_type;
    using extent_type = typename shape_type::extent_type;
    using stride_type = typename shape_type::stride_type;
    using slice_type = typename shape_type::slice_type;
    using bounds_type = typename shape_type::bounds_type;

    array_view_base_t(pointer data, shape_type shape) : m_data{ to_byte_ptr(data) }, m_shape{ shape } { }
    array_view_base_t(const array_view_base_t&) = default;
    array_view_base_t(array_view_base_t&&) noexcept = default;

    array_view_base_t& operator=(const array_view_base_t&) = default;
    array_view_base_t& operator=(array_view_base_t&&) noexcept = default;

    const shape_type& shape() const { return m_shape; }

    array_view_base_t<std::add_const_t<T>, D> as_const() const { return { from_offset(0), m_shape }; }

    operator array_view_base_t<std::add_const_t<T>, D>() const { return as_const(); }

    extent_type extent() const { return m_shape.extent(); }
    stride_type stride() const { return m_shape.stride(); }
    location_type start() const { return m_shape.start(); }
    volume_t volume() const { return m_shape.volume(); }
    bounds_type bounds() const { return m_shape.bounds(); }

    iterator begin() const { return iterator{ from_offset(0), m_shape, 0 }; }

    iterator end() const { return iterator{ from_offset(0), m_shape, volume() }; }

    pointer data() const { return from_offset(0); }

    pointer get(const location_type& loc) const
    {
        const location_type adjusted_loc = m_shape.adjust_location(loc);
        if (!contains(bounds(), adjusted_loc))
        {
            throw std::out_of_range{ format("Location ", loc, " is out of bounds (", extent(), ")") };
        }
        return from_offset(m_shape.flat_offset(adjusted_loc));
    }

    reference operator[](const location_type& loc) const { return *get(loc); }

    array_view_base_t slice(const slice_type& s) const
    {
        const auto [new_shape, new_start] = m_shape.slice(s);
        const flat_offset_t offset = std::accumulate(new_start.begin(), new_start.end(), flat_offset_t{ 0 });
        return array_view_base_t{ from_offset(offset), new_shape };
    }

    array_view_base_t<T, D - 1> sub(std::size_t d, location_base_t n) const
    {
        const location_base_t adjusted_loc = m_shape.dim(d).adjust_location(n);
        if (!contains(m_shape.dim(d).bounds(), adjusted_loc))
        {
            throw std::out_of_range{ format("Index ", n, " is out of bounds (", m_shape.dim(d).extent, ")") };
        }
        const auto offset = adjusted_loc * m_shape.dim(d).stride;
        return array_view_base_t<T, D - 1>{ from_offset(offset), m_shape.erase(d) };
    }

    array_view_base_t<T, D - 1> operator[](location_base_t n) const { return sub(0, n); }

    template <class T_ = T, enable_if_t<!std::is_const_v<T_>> = 0>
    void fill(const value_type& value)
    {
        std::fill(this->begin(), this->end(), value);
    }

    template <class T_ = T, class Range, enable_if_t<!std::is_const_v<T_>> = 0>
    void assign(Range&& range)
    {
        overwrite(this->begin(), this->end(), std::begin(range), std::end(range));
    }

    friend std::ostream& operator<<(std::ostream& os, const array_view_base_t& item) { return os << item.shape(); }

    pointer from_offset(flat_offset_t offset) const { return to_ptr<pointer>(m_data, offset); }

    byte_ptr m_data;
    shape_type m_shape;
};

template <class T>
struct array_view_base_t<T, 1>
{
    using value_type = std::remove_const_t<T>;
    using shape_type = shape_t<1>;
    using pointer = T*;
    using reference = T&;

    using iterator = iterator_interface<detail::flat_iter_impl<T, 1>>;

    using location_type = typename shape_type::location_type;
    using extent_type = typename shape_type::extent_type;
    using stride_type = typename shape_type::stride_type;
    using slice_type = typename shape_type::slice_type;
    using bounds_type = typename shape_type::bounds_type;

    array_view_base_t(pointer data, shape_type shape) : m_data{ to_byte_ptr(data) }, m_shape{ shape } { }

    array_view_base_t(const array_view_base_t&) = default;
    array_view_base_t(array_view_base_t&&) noexcept = default;

    array_view_base_t& operator=(const array_view_base_t&) = default;
    array_view_base_t& operator=(array_view_base_t&&) noexcept = default;

    const shape_type& shape() const { return m_shape; }

    array_view_base_t<std::add_const_t<T>, 1> as_const() const { return { from_offset(0), m_shape }; }

    operator array_view_base_t<std::add_const_t<T>, 1>() const { return as_const(); }

    extent_type extent() const { return m_shape.extent(); }
    stride_type stride() const { return m_shape.stride(); }
    volume_t volume() const { return m_shape.volume(); }
    bounds_type bounds() const { return m_shape.bounds(); }

    iterator begin() const { return iterator{ from_offset(0), m_shape, 0 }; }
    iterator end() const { return iterator{ from_offset(0), m_shape, volume() }; }

    pointer data() const { return from_offset(0); }

    pointer get(location_type loc) const
    {
        const location_type adjusted_loc = m_shape.adjust_location(loc);
        if (!contains(bounds(), adjusted_loc))
        {
            throw std::out_of_range{ format("Location ", loc, " is out of bounds (", extent(), ")") };
        }
        return from_offset(m_shape.flat_offset(adjusted_loc));
    }

    reference operator[](location_type loc) const { return *get(loc); }

    array_view_base_t slice(const slice_type& s) const
    {
        const auto [new_shape, new_start] = m_shape.slice(s);
        return array_view_base_t{ from_offset(new_start), new_shape };
    }

    template <class T_ = T, enable_if_t<!std::is_const_v<T_>> = 0>
    void fill(const value_type& value)
    {
        std::fill(this->begin(), this->end(), value);
    }

    template <class T_ = T, class Range, enable_if_t<!std::is_const_v<T_>> = 0>
    void assign(Range&& range)
    {
        overwrite(this->begin(), this->end(), std::begin(range), std::end(range));
    }

    friend std::ostream& operator<<(std::ostream& os, const array_view_base_t& item) { return os << item.shape(); }

    pointer from_offset(flat_offset_t offset) const { return to_ptr<pointer>(m_data, offset); }

    byte_ptr m_data;
    shape_type m_shape;
};

template <class T, std::size_t D>
using array_view_t = array_view_base_t<const T, D>;

template <class T, std::size_t D>
using array_mut_view_t = array_view_base_t<T, D>;

namespace detail
{

template <class T, class U, std::size_t D>
void copy_from_view(array_mut_view_t<T, D> dst, array_view_t<U, D> src)
{
    if (dst.extent() != src.extent())
    {
        throw std::invalid_argument{ "Source and destination sizes do not match" };
    }

    overwrite(dst, src);
}

}  // namespace detail

template <class T, std::size_t D>
struct array_t
{
    using value_type = T;
    using mut_view_type = array_mut_view_t<T, D>;
    using view_type = array_view_t<T, D>;

    template <std::size_t D_>
    using mut_sub_view_type = array_mut_view_t<T, D_>;

    template <std::size_t D_>
    using sub_view_type = array_view_t<T, D_>;

    using shape_type = typename view_type::shape_type;
    using location_type = typename view_type::location_type;
    using extent_type = typename view_type::extent_type;
    using stride_type = typename view_type::stride_type;
    using bounds_type = typename view_type::bounds_type;
    using slice_type = typename view_type::slice_type;

    using const_pointer = typename view_type::pointer;
    using const_reference = typename view_type::reference;
    using const_iterator = typename view_type::iterator;

    using pointer = typename mut_view_type::pointer;
    using reference = typename mut_view_type::reference;
    using iterator = typename mut_view_type::iterator;

    array_t(const extent_type extent, const T& init = {})
        : m_shape{ shape_type::from_extent(extent, sizeof(T)) }
        , m_data(static_cast<std::size_t>(m_shape.volume()), init)
    {
    }

    template <std::size_t D_ = D, enable_if_t<(D_ == 1)> = 0>
    explicit array_t(std::vector<T> init)
        : m_shape{ shape_type::from_extent(extent_type{ static_cast<extent_base_t>(init.extent()) }, sizeof(T)) }
        , m_data(std::move(init))
    {
    }

    template <class U, enable_if_t<std::is_convertible_v<const U&, T>> = 0>
    array_t(array_view_t<U, D> init)
        : m_shape{ shape_type::from_extent(init.extent(), sizeof(T)) }
        , m_data(static_cast<std::size_t>(m_shape.volume()))
    {
        detail::copy_from_view(mut_view(), init);
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

    extent_type extent() const { return view().extent(); }
    stride_type stride() const { return view().stride(); }
    volume_t volume() const { return view().volume(); }
    bounds_type bounds() const { return view().bounds(); }

    const_reference operator[](const location_type& loc) const { return view()[loc]; }
    reference operator[](const location_type& loc) { return mut_view()[loc]; }

    template <std::size_t D_ = D, enable_if_t<(D_ > 1)> = 0>
    sub_view_type<D_ - 1> operator[](location_base_t n) const
    {
        return view()[n];
    }

    template <std::size_t D_ = D, enable_if_t<(D_ > 1)> = 0>
    mut_sub_view_type<D_ - 1> operator[](location_base_t n)
    {
        return mut_view()[n];
    }

    mut_view_type slice(const slice_type& s) { return mut_view().slice(s); }
    view_type slice(const slice_type& s) const { return view().slice(s); }

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
    inline auto operator()(interval_type dst, interval_type src, location_base_t location) const
        -> std::pair<interval_type, interval_type>
    {
        constexpr auto adjust = [](interval_type interval, extent_base_t lo, extent_base_t up) -> interval_type
        {
            lo = std::clamp(lo, lower(interval), upper(interval));
            up = std::clamp(up, lower(interval), upper(interval));
            up = std::max(up, lo);
            return interval_type{ lo, up };
        };
        const auto new_dst
            = adjust(dst, std::max(lower(dst), lower(src) + location), std::min(upper(dst), upper(src) + location));

        const auto new_src = adjust(src, lower(new_dst) - location, upper(new_dst) - location);

        return { new_src, new_dst };
    }

    template <std::size_t D>
    auto operator()(bounds_t<D> dst, bounds_t<D> src, const location_t<D>& location) const
        -> std::pair<bounds_t<D>, bounds_t<D>>
    {
        bounds_t<D> new_dst = dst;
        bounds_t<D> new_src = src;

        for (std::size_t d = 0; d < D; ++d)
        {
            std::tie(new_src[d], new_dst[d]) = (*this)(dst[d], src[d], location[d]);
        }

        return { new_src, new_dst };
    }
};

static constexpr inline auto adjust_bounds = adjust_bounds_fn{};

struct adjust_copy_bounds_fn
{
    auto operator()(const std::pair<interval_type, interval_type>& dst, const std::pair<interval_type, interval_type>& src)
        const -> std::pair<interval_type, interval_type>
    {
        const auto clip_interval = [](const std::pair<interval_type, interval_type>& pair) -> interval_type
        {
            const auto [bounds, interval] = pair;
            const auto lo = std::max(lower(bounds), lower(interval));
            const auto up = std::clamp(upper(interval), lo, upper(bounds));
            return { lo, up };
        };

        return adjust_bounds(clip_interval(dst), clip_interval(src), lower(dst.second) - lower(src.second));
    }

    template <std::size_t D>
    auto operator()(const std::pair<bounds_t<D>, bounds_t<D>>& dst, const std::pair<bounds_t<D>, bounds_t<D>>& src) const
        -> std::pair<bounds_t<D>, bounds_t<D>>
    {
        bounds_t<D> clipped_dst = {};
        bounds_t<D> clipped_src = {};

        for (std::size_t d = 0; d < D; ++d)
        {
            std::tie(clipped_src[d], clipped_dst[d])
                = (*this)(std::pair{ dst.first[d], dst.second[d] }, std::pair{ src.first[d], src.second[d] });
        }

        return { clipped_src, clipped_dst };
    }
};

static constexpr inline auto adjust_copy_bounds = adjust_copy_bounds_fn{};

struct to_slice_fn
{
    slice_base_t operator()(const interval_t<extent_base_t>& bounds) const
    {
        return slice_base_t{ lower(bounds), upper(bounds) };
    }

    template <std::size_t D>
    slice_t<D> operator()(const bounds_t<D>& bounds) const
    {
        slice_t<D> result = {};
        for (std::size_t d = 0; d < D; ++d)
        {
            result[d] = (*this)(bounds[d]);
        }
        return result;
    }
};

static constexpr inline auto to_slice = to_slice_fn{};

struct copy_fn
{
    template <class T, class U>
    void operator()(array_view_base_t<T, 1> dst, array_view_base_t<U, 1> src) const
    {
        if (dst.extent() != src.extent())
        {
            throw std::invalid_argument{ "Source and destination sizes do not match" };
        }

        for (extent_base_t i = 0; i < dst.extent(); ++i)
        {
            dst[i] = src[i];
        }
    }

    template <class T, class U, std::size_t D>
    void operator()(array_view_base_t<T, D> dst, array_view_base_t<U, D> src) const
    {
        if (dst.extent() != src.extent())
        {
            throw std::invalid_argument{ "Source and destination sizes do not match" };
        }

        for (extent_base_t i = 0; i < dst.shape().dim(0).extent; ++i)
        {
            (*this)(dst[i], src[i]);
        }
    }

    template <class T, class U, std::size_t D>
    void operator()(array_view_base_t<T, D> dst, array_view_base_t<U, D> src, const location_t<D>& location) const
    {
        const auto [src_bounds, dst_bounds] = adjust_bounds(dst.bounds(), src.bounds(), location);
        (*this)(dst.slice(to_slice(dst_bounds)), src.slice(to_slice(src_bounds)));
    }
};

static constexpr inline auto copy = copy_fn{};

}  // namespace mat

}  // namespace zx

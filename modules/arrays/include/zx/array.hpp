#pragma once

#include <array>
#include <optional>
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

struct dim_t
{
    size_base_t size;
    stride_base_t stride;
    location_base_t start = {};

    friend bool operator==(const dim_t& lhs, const dim_t& rhs)
    {
        return std::tie(lhs.size, lhs.stride, lhs.start) == std::tie(rhs.size, rhs.stride, rhs.start);
    }

    friend bool operator!=(const dim_t& lhs, const dim_t& rhs) { return !(lhs == rhs); }

    friend std::ostream& operator<<(std::ostream& os, const dim_t& item)
    {
        return os << "{"
                  << ":size " << item.size << " "
                  << ":stride " << item.stride << " "
                  << ":start " << item.start << "}";
    }
};

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

template <std::size_t D>
struct shape_t
{
    using size_type = zx::mat::vector<size_base_t, D>;
    using stride_type = zx::mat::vector<stride_base_t, D>;
    using location_type = zx::mat::vector<location_base_t, D>;
    using slice_type = zx::mat::vector<slice_base_t, D>;
    using bounds_type = zx::mat::box_shape<size_base_t, D>;

    std::array<dim_t, D> dims{};

    friend bool operator==(const shape_t& lhs, const shape_t& rhs) { return lhs.dims == rhs.dims; }

    friend bool operator!=(const shape_t& lhs, const shape_t& rhs) { return !(lhs == rhs); }

    friend std::ostream& operator<<(std::ostream& os, const shape_t& item)
    {
        os << "{";
        for (std::size_t d = 0; d < D; ++d)
        {
            if (d != 0)
            {
                os << " ";
            }
            os << item.dims[d];
        }
        os << "}";
        return os;
    }

    volume_t volume() const
    {
        volume_t result = 1;
        for (std::size_t d = 0; d < D; ++d)
        {
            result *= dims[d].size;
        }
        return result;
    }

    size_type size() const
    {
        size_type result = {};
        for (std::size_t d = 0; d < D; ++d)
        {
            result[d] = dims[d].size;
        }
        return result;
    }

    stride_type stride() const
    {
        stride_type result = {};
        for (std::size_t d = 0; d < D; ++d)
        {
            result[d] = dims[d].stride;
        }
        return result;
    }

    location_type start() const
    {
        location_type result = {};
        for (std::size_t d = 0; d < D; ++d)
        {
            result[d] = dims[d].start;
        }
        return result;
    }

    bounds_type bounds() const
    {
        bounds_type result = {};
        for (std::size_t d = 0; d < D; ++d)
        {
            result[d] = { 0, dims[d].size };
        }
        return result;
    }

    flat_offset_t flat_offset(const location_type& loc) const
    {
        flat_offset_t result = 0;
        for (std::size_t d = 0; d < D; ++d)
        {
            result += dims[d].start + loc[d] * dims[d].stride;
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
            auto& dim = result.dims[d];
            dim.start = 0;
            dim.size = size[d];
            dim.stride = stride;
            stride *= dim.size;
        }
        return result;
    }

    shape_t slice(const slice_type& s) const
    {
        shape_t result = *this;
        for (std::size_t d = 0; d < D; ++d)
        {
            const dim_t& this_dim = dims[d];
            dim_t& new_dim = result.dims[d];
            location_base_t start = s[d].start.value_or(0);
            location_base_t stop = s[d].stop.value_or(this_dim.size);
            start += (start >= 0 ? 0 : this_dim.size);
            stop += (stop >= 0 ? 0 : this_dim.size);
            const stride_base_t step = s[d].step.value_or(1);
            new_dim.start += start * new_dim.stride;
            new_dim.size = std::max<size_base_t>(0, (stop - start + step - (step > 0 ? 1 : -1)) / step);
            new_dim.stride *= step;
        }
        return result;
    }
};

template <class T>
struct strided_iter
{
    T* m_data;
    stride_base_t m_stride;

    strided_iter() = default;

    strided_iter(T* data, stride_base_t stride) : m_data{ data }, m_stride{ stride } { }

    T& deref() const { return *m_data; }

    bool is_equal(const strided_iter& other) const { return m_data == other.m_data; }
    bool is_less(const strided_iter& other) const { return m_stride > 0 ? m_data < other.m_data : m_data > other.m_data; }

    bool distance_to(const strided_iter& other) const
    {
        assert(m_stride == other.m_stride);
        return (other.m_data - m_data) / m_stride;
    }

    void advance(std::ptrdiff_t offset) { m_data += offset * m_stride; }
};

template <class T, std::size_t D>
struct array_view_t
{
    using value_type = std::remove_const_t<T>;
    using shape_type = shape_t<D>;
    using pointer = T*;
    using reference = T&;

    using iterator = std::conditional_t<D == 1, zx::iterator_interface<strided_iter<T>>, void>;

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

    pointer data() const { return m_data; }

    pointer get(const location_type& loc) const { return data() + m_shape.flat_offset(loc); }
    reference operator[](const location_type& loc) const { return *get(loc); }

    array_view_t slice(const slice_type& s) const { return array_view_t{ data(), m_shape.slice(s) }; }

    template <std::size_t D_ = D, std::enable_if_t<D_ == 1, int> = 0>
    iterator begin() const
    {
        return iterator{ data() + m_shape.dims[0].start, m_shape.dims[0].stride };
    }

    template <std::size_t D_ = D, std::enable_if_t<D_ == 1, int> = 0>
    iterator end() const
    {
        return begin() + m_shape.dims[0].size;
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

    using shape_type = typename view_type::shape_type;
    using location_type = typename view_type::location_type;
    using size_type = typename view_type::size_type;
    using stride_type = typename view_type::stride_type;
    using bounds_type = typename view_type::bounds_type;

    using const_pointer = typename view_type::pointer;
    using const_reference = typename view_type::reference;
    using pointer = typename mut_view_type::pointer;
    using reference = typename mut_view_type::reference;

    array_t(const size_type size) : m_shape{ shape_type::from_size(size) }, m_data{}
    {
        m_data.resize(static_cast<std::size_t>(m_shape.volume()));
    }

    array_t(const array_t&) = default;
    array_t(array_t&&) noexcept = default;
    array_t& operator=(const array_t&) = default;
    array_t& operator=(array_t&&) noexcept = default;

    const shape_type& shape() const { return m_shape; }

    view_type view() const { return { m_data.data(), m_shape }; }
    mut_view_type mut_view() { return { m_data.data(), m_shape }; }

    operator view_type() const { return view(); }

    size_type size() const { return m_shape.size(); }
    stride_type stride() const { return m_shape.stride(); }
    location_type start() const { return m_shape.start(); }
    volume_t volume() const { return m_shape.volume(); }
    bounds_type bounds() const { return m_shape.bounds(); }

    const_pointer data() const { return m_data.data(); }
    pointer data() { return m_data.data(); }

    const_reference operator[](const location_type& loc) const { return view()[loc]; }
    reference operator[](const location_type& loc) { return mut_view()[loc]; }

    friend std::ostream& operator<<(std::ostream& os, const array_t& item) { return os << item.shape(); }

    shape_type m_shape;
    std::vector<T> m_data;
};

}  // namespace arrays

}  // namespace zx

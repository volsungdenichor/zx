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

    dim_t slice(const slice_base_t& s) const
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
        const location_base_t new_start = this->start + actual_start * this->stride;

        return dim_t{ new_size, new_stride, new_start };
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

    using dims_type = zx::mat::vector_t<dim_t, D>;

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
        os << "{";
        for (std::size_t d = 0; d < D; ++d)
        {
            if (d != 0)
            {
                os << " ";
            }
            os << item.m_dims[d];
        }
        os << "}";
        return os;
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

    location_type start() const
    {
        location_type result = {};
        for (std::size_t d = 0; d < D; ++d)
        {
            result[d] = m_dims[d].start;
        }
        return result;
    }

    bounds_type bounds() const
    {
        bounds_type result = {};
        for (std::size_t d = 0; d < D; ++d)
        {
            result[d] = { 0, m_dims[d].size };
        }
        return result;
    }

    flat_offset_t flat_offset(const location_type& loc) const
    {
        flat_offset_t result = 0;
        for (std::size_t d = 0; d < D; ++d)
        {
            result += m_dims[d].start + loc[d] * m_dims[d].stride;
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
            result.m_dims[d] = m_dims[d].slice(s[d]);
        }
        return result;
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

    using dims_type = zx::mat::vector_t<dim_t, 1>;

    dims_type m_dims = {};

    shape_t() = default;
    shape_t(dims_type dims) : m_dims(dims) { }
    shape_t(dim_t dim) : m_dims{ dim } { }

    const dim_t& dim(std::size_t d) const { return m_dims[d]; }

    friend bool operator==(const shape_t& lhs, const shape_t& rhs) { return lhs.m_dims[0] == rhs.m_dims[0]; }
    friend bool operator!=(const shape_t& lhs, const shape_t& rhs) { return !(lhs == rhs); }

    friend std::ostream& operator<<(std::ostream& os, const shape_t& item) { return os << "{" << item.m_dims[0] << "}"; }

    volume_t volume() const { return m_dims[0].size; }

    size_type size() const { return m_dims[0].size; }

    stride_type stride() const { return m_dims[0].stride; }

    location_type start() const { return m_dims[0].start; }

    bounds_type bounds() const { return bounds_type{ 0, size() }; }

    flat_offset_t flat_offset(const location_type& loc) const { return m_dims[0].start + loc * m_dims[0].stride; }

    static shape_t from_size(const size_type& size) { return shape_t{ dim_t{ size, 1, 0 } }; }

    shape_t slice(const slice_type& s) const { return shape_t{ m_dims[0].slice(s) }; }
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

    pointer data() const { return m_data + m_shape.flat_offset(location_type{}); }

    pointer get(const location_type& loc) const { return m_data + m_shape.flat_offset(loc); }

    reference operator[](const location_type& loc) const { return *get(loc); }

    array_view_t slice(const slice_type& s) const { return array_view_t{ m_data, m_shape.slice(s) }; }

    template <std::size_t D_ = D, std::enable_if_t<(D_ == 1), int> = 0>
    iterator begin() const
    {
        return iterator{ m_data + m_shape.dim(0).start, m_shape.dim(0).stride };
    }

    template <std::size_t D_ = D, std::enable_if_t<(D_ == 1), int> = 0>
    iterator end() const
    {
        return begin() + volume();
    }

    template <std::size_t D_ = D, std::enable_if_t<(D_ > 1), int> = 0>
    array_view_t<T, D - 1> sub(std::size_t d, location_base_t n) const
    {
        const auto offset = m_shape.dim(d).start + n * m_shape.dim(d).stride;
        return array_view_t<T, D - 1>{ m_data + offset, shape_t<D - 1>{ mat::erase(m_shape.m_dims, d) } };
    }

    template <std::size_t D_ = D, std::enable_if_t<(D_ > 1), int> = 0>
    array_view_t<T, D - 1> operator[](location_base_t n) const
    {
        return sub(0, n);
    }

    template <class T_ = T, std::enable_if_t<!std::is_const_v<T_>, int> = 0>
    void fill(const value_type& value)
    {
        if constexpr (D == 1)
        {
            std::fill(begin(), end(), value);
        }
        else
        {
            for (std::size_t i = 0; i < m_shape.dim(0).size; ++i)
            {
                (*this)[i].fill(value);
            }
        }
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

    using const_pointer = typename view_type::pointer;
    using const_reference = typename view_type::reference;
    using const_iterator = typename view_type::iterator;

    using pointer = typename mut_view_type::pointer;
    using reference = typename mut_view_type::reference;
    using iterator = typename mut_view_type::iterator;

    array_t(const size_type size, const T& init = {}) : m_shape{ shape_type::from_size(size) }, m_data{}
    {
        m_data.resize(static_cast<std::size_t>(m_shape.volume()), init);
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

    size_type size() const { return m_shape.size(); }
    stride_type stride() const { return m_shape.stride(); }
    location_type start() const { return m_shape.start(); }
    volume_t volume() const { return m_shape.volume(); }
    bounds_type bounds() const { return m_shape.bounds(); }

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

    iterator begin() { return mut_view().begin(); }
    iterator end() { return mut_view().end(); }

    const_iterator begin() const { return view().begin(); }
    const_iterator end() const { return view().end(); }

    friend std::ostream& operator<<(std::ostream& os, const array_t& item) { return os << item.shape(); }

    shape_type m_shape;
    std::vector<T> m_data;
};

}  // namespace arrays

}  // namespace zx

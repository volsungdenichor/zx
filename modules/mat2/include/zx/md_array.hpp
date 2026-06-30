#pragma once

#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

namespace zx
{
namespace mat2
{

struct flat_offset_t
{
    std::ptrdiff_t m_value;

    constexpr explicit flat_offset_t(std::ptrdiff_t value) : m_value(value) { }

    constexpr friend flat_offset_t operator+(const flat_offset_t& lhs, const flat_offset_t& rhs)
    {
        return flat_offset_t{ lhs.m_value + rhs.m_value };
    }

    constexpr friend bool operator==(const flat_offset_t& lhs, const flat_offset_t& rhs)
    {
        return lhs.m_value == rhs.m_value;
    }
    constexpr friend bool operator!=(const flat_offset_t& lhs, const flat_offset_t& rhs) { return lhs != rhs; }
    constexpr friend bool operator<(const flat_offset_t& lhs, const flat_offset_t& rhs) { return lhs.m_value < rhs.m_value; }
    constexpr friend bool operator<=(const flat_offset_t& lhs, const flat_offset_t& rhs)
    {
        return lhs.m_value <= rhs.m_value;
    }
    constexpr friend bool operator>(const flat_offset_t& lhs, const flat_offset_t& rhs) { return lhs.m_value > rhs.m_value; }
    constexpr friend bool operator>=(const flat_offset_t& lhs, const flat_offset_t& rhs)
    {
        return lhs.m_value >= rhs.m_value;
    }

    friend std::ostream& operator<<(std::ostream& os, const flat_offset_t& item) { return os << item.m_value; }
};

struct ptr_t
{
    std::uint8_t* m_ptr;

    template <class T>
    constexpr explicit ptr_t(T* ptr) : m_ptr(const_cast<std::uint8_t*>(reinterpret_cast<const std::uint8_t*>(ptr)))
    {
    }

    constexpr friend ptr_t operator+(ptr_t ptr, flat_offset_t offset) { return ptr_t{ ptr.m_ptr + offset.m_value }; }

    template <class T>
    constexpr T* as() const
    {
        return reinterpret_cast<T*>(m_ptr);
    }
};

using size_value_t = std::ptrdiff_t;
using stride_value_t = std::ptrdiff_t;
using location_value_t = std::ptrdiff_t;
using volume_value_t = std::ptrdiff_t;

template <class T = location_value_t>
struct interval_base_t
{
    using value_type = T;
    value_type lower;
    value_type upper;

    friend bool operator==(const interval_base_t& lhs, const interval_base_t& rhs)
    {
        return std::tie(lhs.lower, lhs.upper) == std::tie(rhs.lower, rhs.upper);
    }

    friend bool operator!=(const interval_base_t& lhs, const interval_base_t& rhs) { return !(lhs == rhs); }

    friend std::ostream& operator<<(std::ostream& os, const interval_base_t& item)
    {
        return os << "[" << item.lower << " " << item.upper << ")";
    }
};

struct slice_base_t
{
    std::optional<location_value_t> start = {};
    std::optional<location_value_t> stop = {};
    std::optional<stride_value_t> step = {};

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

template <class T>
constexpr T dynamic = std::numeric_limits<T>::max();

template <size_value_t Size = dynamic<size_value_t>, stride_value_t Stride = dynamic<stride_value_t>>
struct dim_t
{
    dim_t() = default;

    constexpr dim_t(size_value_t, stride_value_t) { }

    constexpr size_value_t size() const { return Size; }

    constexpr stride_value_t stride() const { return Stride; }

    constexpr flat_offset_t flat_offset(location_value_t loc) const { return flat_offset_t{ loc * stride() }; }
};

template <>
struct dim_t<dynamic<size_value_t>, dynamic<stride_value_t>>
{
    size_value_t m_size;
    stride_value_t m_stride;

    constexpr dim_t(size_value_t size, stride_value_t stride) : m_size(size), m_stride(stride) { }

    template <size_value_t Size, stride_value_t Stride>
    constexpr dim_t(const dim_t<Size, Stride>& other) : m_size(other.size())
                                                      , m_stride(other.stride())
    {
    }

    constexpr size_value_t size() const { return m_size; }

    constexpr stride_value_t stride() const { return m_stride; }

    constexpr flat_offset_t flat_offset(location_value_t loc) const { return flat_offset_t{ loc * stride() }; }
};

using dynamic_dim_t = dim_t<dynamic<size_value_t>, dynamic<stride_value_t>>;

enum class dim_type_t : std::uint8_t
{
    static_dim,
    dynamic_dim
};

template <class T>
struct get_dim_type;

template <size_value_t Size, stride_value_t Stride>
struct get_dim_type<dim_t<Size, Stride>> : std::integral_constant<dim_type_t, dim_type_t::static_dim>
{
};

template <>
struct get_dim_type<dynamic_dim_t> : std::integral_constant<dim_type_t, dim_type_t::dynamic_dim>
{
};

inline std::pair<dynamic_dim_t, location_value_t> do_slice(const dynamic_dim_t& dim, const slice_base_t& s)
{
    const auto clamp = [&](location_value_t value, location_value_t init) -> location_value_t
    { return std::max(init, std::min(value, dim.size() + init)); };

    const stride_value_t actual_step = s.step.value_or(stride_value_t{ 1 });
    location_value_t actual_start, actual_stop;

    if (actual_step > 0)
    {
        actual_start = s.start.value_or(location_value_t{ 0 });
        actual_stop = s.stop.value_or(location_value_t{ dim.size() });

        actual_start = clamp(actual_start + (actual_start < 0 ? dim.size() : 0), 0);
        actual_stop = clamp(actual_stop + (actual_stop < 0 ? dim.size() : 0), 0);
    }
    else
    {
        actual_start = s.start.value_or(location_value_t{ dim.size() - 1 });
        actual_stop = s.stop.value_or(location_value_t{ -dim.size() - 1 });

        actual_start = clamp(actual_start + (actual_start < 0 ? dim.size() : 0), -1);
        actual_stop = clamp(actual_stop + (actual_stop < 0 ? dim.size() : 0), -1);
    }

    const size_value_t new_size
        = actual_step > 0 ? std::max(location_value_t(0), (actual_stop - actual_start + actual_step - 1) / actual_step)
                          : std::max(location_value_t(0), (actual_start - actual_stop - actual_step - 1) / (-actual_step));

    const stride_value_t new_stride = dim.stride() * actual_step;
    const location_value_t new_start = actual_start * dim.stride();

    return { dim_t{ new_size, new_stride }, new_start };
}

template <class... Dims>
struct shape_t
{
    using storage_type = std::tuple<Dims...>;
    using loc_type = std::array<location_value_t, sizeof...(Dims)>;

    static constexpr inline std::size_t dims_count = sizeof...(Dims);
    storage_type m_storage;

    static constexpr bool is_static() { return ((get_dim_type<Dims>::value == dim_type_t::static_dim) && ...); }

    template <bool S = is_static(), std::enable_if_t<S, int> = 0>
    constexpr shape_t() : m_storage{}
    {
    }

    constexpr explicit shape_t(Dims... dims) : m_storage(std::move(dims)...) { }

    template <std::size_t N>
    constexpr dynamic_dim_t dim() const
    {
        return dynamic_dim_t{ std::get<N>(m_storage) };
    }

    constexpr flat_offset_t flat_offset(const loc_type& loc) const
    {
        return flat_offset(loc, std::make_index_sequence<dims_count>{});
    }

    constexpr volume_value_t volume() const
    {
        volume_value_t vol = 1;
        std::apply([&vol](const auto&... dims) { ((vol *= dims.size()), ...); }, m_storage);
        return vol;
    }

    template <class Func>
    constexpr auto apply(Func&& func) const
    {
        return std::apply(std::forward<Func>(func), m_storage);
    }

private:
    template <std::size_t... Is>
    constexpr flat_offset_t flat_offset(const loc_type& loc, std::index_sequence<Is...>) const
    {
        return ((std::get<Is>(m_storage).flat_offset(loc[Is])) + ...);
    }
};

template <class T>
struct get_shape_type;

template <class... Dims>
struct get_shape_type<shape_t<Dims...>>
    : std::integral_constant<dim_type_t, (shape_t<Dims...>::is_static() ? dim_type_t::static_dim : dim_type_t::dynamic_dim)>
{
};

template <dim_type_t Type, class Shape>
struct shape_holder_t;

template <dim_type_t Type, class Shape, class T>
struct storage_holder_t;

template <class Shape>
struct shape_holder_t<dim_type_t::static_dim, Shape>
{
    shape_holder_t() = default;

    explicit shape_holder_t(const Shape&) { }

    const Shape& shape() const
    {
        static const Shape instance = {};
        return instance;
    }
};

template <class Shape>
struct shape_holder_t<dim_type_t::dynamic_dim, Shape>
{
    Shape m_shape;

    shape_holder_t(Shape shape) : m_shape(std::move(shape)) { }

    const Shape& shape() const { return m_shape; }
};

template <class Shape, class T>
struct storage_holder_t<dim_type_t::static_dim, Shape, T>
{
    using storage_type = std::array<T, Shape{}.volume()>;

    storage_type m_storage;

    storage_holder_t() : m_storage{} { }

    explicit storage_holder_t(const Shape&) : m_storage{} { }

    storage_type& storage() { return m_storage; }

    const storage_type& storage() const { return m_storage; }
};

template <class Shape, class T>
struct storage_holder_t<dim_type_t::dynamic_dim, Shape, T>
{
    using storage_type = std::vector<T>;

    storage_type m_storage;

    explicit storage_holder_t(const Shape& shape) : m_storage(static_cast<std::size_t>(shape.volume())) { }

    storage_type& storage() { return m_storage; }

    const storage_type& storage() const { return m_storage; }
};

template <class Shape, class T>
struct array_view_t;

template <class Shape, class T>
struct array_t;

template <class Dim0, class T>
struct array_t<shape_t<Dim0>, T> : private shape_holder_t<get_shape_type<shape_t<Dim0>>::value, shape_t<Dim0>>,
                                   private storage_holder_t<get_shape_type<shape_t<Dim0>>::value, shape_t<Dim0>, T>
{
    using shape_type = shape_t<Dim0>;
    using shape_holder_type = shape_holder_t<get_shape_type<shape_type>::value, shape_type>;
    using storage_holder_type = storage_holder_t<get_shape_type<shape_type>::value, shape_type, T>;

    using view_type = array_view_t<shape_type, T>;
    using const_view_type = array_view_t<shape_type, const T>;

    using value_type = T;
    using const_reference = typename const_view_type::reference;
    using reference = typename view_type::reference;

    using location_type = location_value_t;
    using extent_type = size_value_t;
    using volume_type = volume_value_t;
    using bounds_type = interval_base_t<location_value_t>;
    using slice_type = slice_base_t;

    using shape_holder_type::shape;
    using storage_holder_type::storage;

    array_t(shape_type s = {}) : shape_holder_type(std::move(s)), storage_holder_type(this->shape()) { }

    template <class... Tail, bool S = shape_type::is_static(), std::enable_if_t<S, int> = 0>
    array_t(value_type head, Tail... tail) : array_t{}
    {
        constexpr std::size_t values_count = 1 + sizeof...(Tail);
        static_assert(
            values_count == static_cast<std::size_t>(shape_type{}.volume()),
            "array_t: number of provided values must match static array volume");

        std::size_t i = 0;
        for (value_type item : { std::move(head), static_cast<value_type>(std::move(tail))... })
        {
            this->storage()[i++] = std::move(item);
        }
    }

    template <bool S = shape_type::is_static(), std::enable_if_t<S, int> = 0>
    constexpr operator std::array<value_type, static_cast<std::size_t>(shape_type{}.volume())>() const
    {
        std::array<value_type, static_cast<std::size_t>(shape_type{}.volume())> result{};
        for (std::size_t i = 0; i < result.size(); ++i)
        {
            result[i] = this->storage()[i];
        }
        return result;
    }

    const_view_type view() const { return const_view_type{ this->shape(), this->storage().data() }; }
    view_type view() { return view_type{ this->shape(), this->storage().data() }; }

    volume_type volume() const { return this->shape().volume(); }
    extent_type extents() const { return this->volume(); }
    bounds_type bounds() const { return bounds_type{ 0, this->extents() }; }

    const_reference operator[](const location_type& loc) const { return view()[{ loc }]; }
    reference operator[](const location_type& loc) { return view()[{ loc }]; }

    friend std::ostream& operator<<(std::ostream& os, const array_t& item)
    {
        os << "[";
        for (location_type i = 0; i < static_cast<location_type>(item.volume()); ++i)
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

    friend bool operator==(const array_t& lhs, const array_t& rhs)
    {
        if (lhs.volume() != rhs.volume())
        {
            return false;
        }

        for (location_type i = 0; i < static_cast<location_type>(lhs.volume()); ++i)
        {
            if (lhs[i] != rhs[i])
            {
                return false;
            }
        }

        return true;
    }

    friend bool operator!=(const array_t& lhs, const array_t& rhs) { return !(lhs == rhs); }
};

template <std::size_t D, class T>
using dense_vector_t = array_t<shape_t<dim_t<static_cast<size_value_t>(D), static_cast<stride_value_t>(sizeof(T))>>, T>;

template <class Shape, class T>
struct array_view_t : private shape_holder_t<get_shape_type<Shape>::value, Shape>
{
    using shape_type = Shape;
    using value_type = T;
    using pointer = T*;
    using reference = T&;

    using location_type = dense_vector_t<shape_type::dims_count, location_value_t>;
    using extent_type = dense_vector_t<shape_type::dims_count, size_value_t>;
    using volume_type = volume_value_t;
    using bounds_type = dense_vector_t<shape_type::dims_count, interval_base_t<location_value_t>>;
    using slice_type = dense_vector_t<shape_type::dims_count, slice_base_t>;

    using shape_holder_type = shape_holder_t<get_shape_type<shape_type>::value, shape_type>;

    using shape_holder_type::shape;
    ptr_t m_data;

    array_view_t(shape_type s, pointer data) : shape_holder_type(std::move(s)), m_data(data) { }

    pointer get(const location_type& loc) const { return (m_data + this->shape().flat_offset(loc)).template as<T>(); }

    volume_type volume() const { return this->shape().volume(); }

    extent_type extents() const
    {
        return shape().apply([](const auto&... dims) { return extent_type{ static_cast<size_value_t>(dims.size())... }; });
    }

    bounds_type bounds() const
    {
        return shape().apply(
            [](const auto&... dims) {
                return bounds_type{ interval_base_t<location_value_t>{ 0, static_cast<location_value_t>(dims.size()) }... };
            });
    }

    reference operator[](const location_type& loc) const { return *get(loc); }
};

template <class Shape, class T>
struct array_t : private shape_holder_t<get_shape_type<Shape>::value, Shape>,
                 private storage_holder_t<get_shape_type<Shape>::value, Shape, T>
{
    using shape_type = Shape;
    using shape_holder_type = shape_holder_t<get_shape_type<shape_type>::value, shape_type>;
    using storage_holder_type = storage_holder_t<get_shape_type<shape_type>::value, shape_type, T>;

    using view_type = array_view_t<shape_type, T>;
    using const_view_type = array_view_t<shape_type, const T>;

    using value_type = T;
    using const_reference = typename const_view_type::reference;
    using reference = typename view_type::reference;

    using location_type = typename const_view_type::location_type;
    using extent_type = typename const_view_type::extent_type;
    using volume_type = typename const_view_type::volume_type;
    using bounds_type = typename const_view_type::bounds_type;
    using slice_type = typename const_view_type::slice_type;

    using shape_holder_type::shape;
    using storage_holder_type::storage;

    array_t(shape_type s = {}) : shape_holder_type(std::move(s)), storage_holder_type(this->shape()) { }

    const_view_type view() const { return const_view_type{ this->shape(), this->storage().data() }; }
    view_type view() { return view_type{ this->shape(), this->storage().data() }; }

    volume_type volume() const { return this->view().volume(); }
    extent_type extents() const { return this->view().extents(); }
    bounds_type bounds() const { return this->view().bounds(); }

    const_reference operator[](const location_type& loc) const { return view()[loc]; }
    reference operator[](const location_type& loc) { return view()[loc]; }
};

}  // namespace mat2
}  // namespace zx

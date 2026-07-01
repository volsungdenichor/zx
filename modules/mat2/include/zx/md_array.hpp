#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <type_traits>
#include <vector>

namespace zx
{
namespace mat2
{

template <std::size_t D, class T>
struct vec_t : public std::array<T, D>
{
    using base_t = std::array<T, D>;
    using base_t::base_t;

    constexpr vec_t() : base_t{} { std::fill(this->begin(), this->end(), T{}); }

    template <class... Tail>
    constexpr vec_t(T head, Tail... tail) : base_t{ { head, static_cast<T>(tail)... } }
    {
        static_assert(sizeof...(tail) + 1 == D, "Invalid number of arguments to vec_t constructor");
    }

    template <class U>
    constexpr vec_t(const vec_t<D, U>& other) : base_t{}
    {
        std::copy(other.begin(), other.end(), this->begin());
    }

    friend std::ostream& operator<<(std::ostream& os, const vec_t& item)
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

    friend constexpr bool operator==(const vec_t& lhs, const vec_t& rhs)
    {
        return std::equal(lhs.begin(), lhs.end(), rhs.begin());
    }
    friend constexpr bool operator!=(const vec_t& lhs, const vec_t& rhs) { return !(lhs == rhs); }
};

template <class T>
vec_t(T) -> vec_t<1, T>;

template <class T>
vec_t(T, T) -> vec_t<2, T>;

template <class T>
vec_t(T, T, T) -> vec_t<3, T>;

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

using extent_value_t = std::ptrdiff_t;
using stride_value_t = std::ptrdiff_t;
using location_value_t = std::ptrdiff_t;
using volume_value_t = std::ptrdiff_t;

struct interval_base_t
{
    location_value_t lower;
    location_value_t upper;

    friend constexpr bool operator==(const interval_base_t& lhs, const interval_base_t& rhs)
    {
        return std::tie(lhs.lower, lhs.upper) == std::tie(rhs.lower, rhs.upper);
    }

    friend constexpr bool operator!=(const interval_base_t& lhs, const interval_base_t& rhs) { return !(lhs == rhs); }

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

    friend constexpr bool operator==(const slice_base_t& lhs, const slice_base_t& rhs)
    {
        return std::tie(lhs.start, lhs.stop, lhs.step) == std::tie(rhs.start, rhs.stop, rhs.step);
    }

    friend constexpr bool operator!=(const slice_base_t& lhs, const slice_base_t& rhs) { return !(lhs == rhs); }

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

template <extent_value_t Extent = dynamic<extent_value_t>, stride_value_t Stride = dynamic<stride_value_t>>
struct dim_t
{
    dim_t() = default;

    constexpr dim_t(extent_value_t, stride_value_t) { }

    constexpr extent_value_t extent() const { return Extent; }

    constexpr stride_value_t stride() const { return Stride; }

    constexpr flat_offset_t flat_offset(location_value_t loc) const { return flat_offset_t{ loc * stride() }; }

    friend std::ostream& operator<<(std::ostream& os, const dim_t& item)
    {
        return os << "{"
                  << ":extent " << item.extent() << " "
                  << ":stride " << item.stride() << "}";
    }
};

template <>
struct dim_t<dynamic<extent_value_t>, dynamic<stride_value_t>>
{
    extent_value_t m_extent;
    stride_value_t m_stride;

    constexpr dim_t(extent_value_t extent, stride_value_t stride) : m_extent(extent), m_stride(stride) { }

    template <extent_value_t Extent, stride_value_t Stride>
    constexpr dim_t(const dim_t<Extent, Stride>& other) : m_extent(other.extent())
                                                        , m_stride(other.stride())
    {
    }

    constexpr extent_value_t extent() const { return m_extent; }

    constexpr stride_value_t stride() const { return m_stride; }

    constexpr flat_offset_t flat_offset(location_value_t loc) const { return flat_offset_t{ loc * stride() }; }

    friend std::ostream& operator<<(std::ostream& os, const dim_t& item)
    {
        return os << "{"
                  << ":extent " << item.extent() << " "
                  << ":stride " << item.stride() << "}";
    }
};

using dynamic_dim_t = dim_t<dynamic<extent_value_t>, dynamic<stride_value_t>>;

template <extent_value_t Extent = dynamic<extent_value_t>, stride_value_t Stride = dynamic<stride_value_t>>
constexpr dynamic_dim_t to_dynamic_dim(dim_t<Extent, Stride> dim)
{
    return dynamic_dim_t{ dim.extent(), dim.stride() };
}

enum class dim_type_t : std::uint8_t
{
    static_dim,
    dynamic_dim
};

template <class T>
struct get_dim_type;

template <extent_value_t Extent, stride_value_t Stride>
struct get_dim_type<dim_t<Extent, Stride>> : std::integral_constant<dim_type_t, dim_type_t::static_dim>
{
};

template <>
struct get_dim_type<dynamic_dim_t> : std::integral_constant<dim_type_t, dim_type_t::dynamic_dim>
{
};

inline constexpr std::pair<dynamic_dim_t, location_value_t> do_slice(const dynamic_dim_t& dim, const slice_base_t& s)
{
    const auto clamp = [&](location_value_t value, location_value_t init) -> location_value_t
    { return std::max(init, std::min(value, dim.extent() + init)); };

    const stride_value_t actual_step = s.step.value_or(stride_value_t{ 1 });
    location_value_t actual_start = 0;
    location_value_t actual_stop = 0;

    if (actual_step > 0)
    {
        actual_start = s.start.value_or(location_value_t{ 0 });
        actual_stop = s.stop.value_or(location_value_t{ dim.extent() });

        actual_start = clamp(actual_start + (actual_start < 0 ? dim.extent() : 0), 0);
        actual_stop = clamp(actual_stop + (actual_stop < 0 ? dim.extent() : 0), 0);
    }
    else
    {
        actual_start = s.start.value_or(location_value_t{ dim.extent() - 1 });
        actual_stop = s.stop.value_or(location_value_t{ -dim.extent() - 1 });

        actual_start = clamp(actual_start + (actual_start < 0 ? dim.extent() : 0), -1);
        actual_stop = clamp(actual_stop + (actual_stop < 0 ? dim.extent() : 0), -1);
    }

    const extent_value_t new_size
        = actual_step > 0 ? std::max(location_value_t(0), (actual_stop - actual_start + actual_step - 1) / actual_step)
                          : std::max(location_value_t(0), (actual_start - actual_stop - actual_step - 1) / (-actual_step));

    const stride_value_t new_stride = dim.stride() * actual_step;
    const location_value_t new_start = actual_start * dim.stride();

    return { dim_t{ new_size, new_stride }, new_start };
}

template <class... Dims>
struct shape_t
{
    static constexpr inline std::size_t dims_count = sizeof...(Dims);

    using storage_type = std::tuple<Dims...>;
    using dynamic_shape_type = shape_t<std::conditional_t<true, dynamic_dim_t, Dims>...>;
    using loc_type = vec_t<dims_count, location_value_t>;
    using extent_type = vec_t<dims_count, extent_value_t>;
    using stride_type = vec_t<dims_count, stride_value_t>;

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

    constexpr extent_type extents() const
    {
        return apply([](const auto&... dims) { return extent_type{ dims.extent()... }; });
    }

    constexpr stride_type stride() const
    {
        return apply([](const auto&... dims) { return stride_type{ dims.stride()... }; });
    }

    constexpr dynamic_shape_type to_dynamic_shape() const
    {
        return apply([](const auto&... dims) { return dynamic_shape_type{ to_dynamic_dim(dims)... }; });
    }

    constexpr flat_offset_t flat_offset(const loc_type& loc) const
    {
        return flat_offset(loc, std::make_index_sequence<dims_count>{});
    }

    constexpr volume_value_t volume() const
    {
        volume_value_t vol = 1;
        apply([&vol](const auto&... dims) { ((vol *= dims.extent()), ...); });
        return vol;
    }

    template <class Func>
    constexpr auto apply(Func&& func) const
    {
        return std::apply(std::forward<Func>(func), m_storage);
    }

    friend std::ostream& operator<<(std::ostream& os, const shape_t& item)
    {
        return os << "{ :size " << item.extents() << " :stride " << item.stride() << " }";
    }

private:
    template <std::size_t... Is>
    constexpr flat_offset_t flat_offset(const loc_type& loc, std::index_sequence<Is...>) const
    {
        return ((std::get<Is>(m_storage).flat_offset(loc[Is])) + ...);
    }
};

template <extent_value_t... Dims>
struct extent_t
{
};

namespace detail
{

template <stride_value_t ElementSize, extent_value_t... Dims>
struct static_shape_builder_impl;

template <stride_value_t ElementSize, extent_value_t... Dims>
struct static_shape_builder_impl
{
    static constexpr std::size_t dims_count = sizeof...(Dims);
    static_assert(dims_count > 0, "shape_builder_t requires at least one dimension");

    static constexpr std::array<extent_value_t, dims_count> extents{ Dims... };

    template <std::size_t I>
    static constexpr extent_value_t extent_at()
    {
        return extents[I];
    }

    template <std::size_t I>
    static constexpr stride_value_t stride_at()
    {
        stride_value_t stride = ElementSize;
        for (std::size_t j = I + 1; j < dims_count; ++j)
        {
            stride *= extents[j];
        }
        return stride;
    }

    template <std::size_t... Is>
    static auto make_shape_type(std::index_sequence<Is...>) -> shape_t<dim_t<extent_at<Is>(), stride_at<Is>()>...>;

    using type = decltype(make_shape_type(std::make_index_sequence<dims_count>{}));
};

}  // namespace detail

template <stride_value_t ElementSize, extent_value_t... Dims>
struct shape_builder_t
{
    using type = typename detail::static_shape_builder_impl<ElementSize, Dims...>::type;
};

template <stride_value_t ElementSize, class Size>
struct shape_builder_from_size_t;

template <stride_value_t ElementSize, extent_value_t... Dims>
struct shape_builder_from_size_t<ElementSize, extent_t<Dims...>> : shape_builder_t<ElementSize, Dims...>
{
};

template <class Extents, stride_value_t ElementSize>
using shape_from_extent_t = typename shape_builder_from_size_t<ElementSize, Extents>::type;

inline auto shape_from_extent(extent_value_t extent, stride_value_t element_size) -> shape_t<dynamic_dim_t>
{
    return shape_t<dynamic_dim_t>{ dynamic_dim_t{ extent, element_size } };
}

inline auto shape_from_extent(const vec_t<2, extent_value_t>& extent, stride_value_t element_size)
    -> shape_t<dynamic_dim_t, dynamic_dim_t>
{
    return shape_t<dynamic_dim_t, dynamic_dim_t>{ dynamic_dim_t{ extent[0], element_size * extent[1] },
                                                  dynamic_dim_t{ extent[1], element_size } };
}

inline auto shape_from_extent(const vec_t<3, extent_value_t>& extent, stride_value_t element_size)
    -> shape_t<dynamic_dim_t, dynamic_dim_t, dynamic_dim_t>
{
    return shape_t<dynamic_dim_t, dynamic_dim_t, dynamic_dim_t>{ dynamic_dim_t{ extent[0],
                                                                                element_size * extent[1] * extent[2] },
                                                                 dynamic_dim_t{ extent[1], element_size * extent[2] },
                                                                 dynamic_dim_t{ extent[2], element_size } };
}

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

    constexpr explicit shape_holder_t(const Shape&) { }

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

    constexpr shape_holder_t(Shape shape) : m_shape(std::move(shape)) { }

    constexpr const Shape& shape() const { return m_shape; }
};

template <class Shape, class T>
struct storage_holder_t<dim_type_t::static_dim, Shape, T>
{
    using storage_type = std::array<T, Shape{}.volume()>;

    storage_type m_storage;

    constexpr storage_holder_t() : m_storage{} { }

    constexpr explicit storage_holder_t(const Shape&) : m_storage{} { }

    constexpr storage_type& storage() { return m_storage; }

    constexpr const storage_type& storage() const { return m_storage; }
};

template <class Shape, class T>
struct storage_holder_t<dim_type_t::dynamic_dim, Shape, T>
{
    using storage_type = std::vector<T>;

    storage_type m_storage;

    constexpr explicit storage_holder_t(const Shape& shape) : m_storage(static_cast<std::size_t>(shape.volume())) { }

    constexpr storage_type& storage() { return m_storage; }

    constexpr const storage_type& storage() const { return m_storage; }
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
    using extent_type = extent_value_t;
    using volume_type = volume_value_t;
    using bounds_type = interval_base_t;
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
    constexpr operator vec_t<static_cast<std::size_t>(shape_type{}.volume()), value_type>() const
    {
        vec_t<static_cast<std::size_t>(shape_type{}.volume()), value_type> result{};
        for (std::size_t i = 0; i < result.size(); ++i)
        {
            result[i] = this->storage()[i];
        }
        return result;
    }

    const_view_type view() const { return const_view_type{ this->shape(), this->storage().data() }; }
    view_type view() { return view_type{ this->shape(), this->storage().data() }; }

    volume_type volume() const { return view().volume(); }
    extent_type extents() const { return view().extents()[0]; }
    bounds_type bounds() const { return view().bounds()[0]; }

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
using dense_vector_t = array_t<shape_t<dim_t<static_cast<extent_value_t>(D), static_cast<stride_value_t>(sizeof(T))>>, T>;

template <class Shape, class T>
struct array_view_t : private shape_holder_t<get_shape_type<Shape>::value, Shape>
{
    using shape_type = Shape;
    using value_type = T;
    using pointer = T*;
    using reference = T&;

    using location_type = dense_vector_t<shape_type::dims_count, location_value_t>;
    using extent_type = dense_vector_t<shape_type::dims_count, extent_value_t>;
    using volume_type = volume_value_t;
    using bounds_type = dense_vector_t<shape_type::dims_count, interval_base_t>;
    using slice_type = dense_vector_t<shape_type::dims_count, slice_base_t>;
    using dynamic_shape_type = typename shape_type::dynamic_shape_type;
    using dynamic_view_type = array_view_t<dynamic_shape_type, T>;

    using shape_holder_type = shape_holder_t<get_shape_type<shape_type>::value, shape_type>;

    using shape_holder_type::shape;
    ptr_t m_data;

    array_view_t(shape_type s, pointer data) : shape_holder_type(std::move(s)), m_data(data) { }

    pointer get(const location_type& loc) const { return (m_data + this->shape().flat_offset(loc)).template as<T>(); }

    volume_type volume() const { return this->shape().volume(); }

    extent_type extents() const
    {
        return shape().apply([](const auto&... dims)
                             { return extent_type{ static_cast<extent_value_t>(dims.extent())... }; });
    }

    bounds_type bounds() const
    {
        return shape().apply(
            [](const auto&... dims) {
                return bounds_type{ interval_base_t{ 0, static_cast<location_value_t>(dims.extent()) }... };
            });
    }

    reference operator[](const location_type& loc) const { return *get(loc); }

    dynamic_view_type slice(const slice_type& s) const
    {
        location_value_t i = 0;
        flat_offset_t total_offset{ 0 };
        const auto new_shape = this->shape().apply(
            [&](const auto&... dims)
            {
                return dynamic_shape_type{ (
                    [&]()
                    {
                        const auto [new_dim, new_start] = do_slice(to_dynamic_dim(dims), s[i++]);
                        total_offset = total_offset + flat_offset_t{ new_start };
                        return new_dim;
                    }())... };
            });

        return dynamic_view_type{ new_shape, (m_data + total_offset).template as<T>() };
    }

    friend std::ostream& operator<<(std::ostream& os, const array_view_t& item) { return os << item.shape(); }
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

    using dynamic_view_type = typename view_type::dynamic_view_type;
    using dynamic_const_view_type = typename const_view_type::dynamic_view_type;

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

    dynamic_const_view_type slice(const slice_type& s) const { return view().slice(s); }
    dynamic_view_type slice(const slice_type& s) { return view().slice(s); }
};

template <std::size_t R, std::size_t C, class T>
using dense_matrix_t = array_t<
    shape_t<
        dim_t<static_cast<extent_value_t>(R), static_cast<stride_value_t>(sizeof(T) * C)>,
        dim_t<static_cast<extent_value_t>(C), static_cast<stride_value_t>(sizeof(T))>>,
    T>;

template <class T>
struct is_dense_vector : std::false_type
{
};

template <extent_value_t E, stride_value_t S, class T>
struct is_dense_vector<array_t<shape_t<dim_t<E, S>>, T>> : std::true_type
{
};

template <class T>
struct is_dense_matrix : std::false_type
{
};

template <extent_value_t R, stride_value_t RS, extent_value_t C, stride_value_t CS, class T>
struct is_dense_matrix<array_t<shape_t<dim_t<R, RS>, dim_t<C, CS>>, T>> : std::true_type
{
};

template <class T>
struct is_array_vector_t : std::false_type
{
};

template <class Dim0, class T>
struct is_array_vector_t<array_t<shape_t<Dim0>, T>> : std::true_type
{
};

template <class T>
using is_scalar_t = std::bool_constant<!is_array_vector_t<std::decay_t<T>>::value>;

template <class Dim0, class T>
constexpr auto operator+(const array_t<shape_t<Dim0>, T>& item) -> array_t<shape_t<Dim0>, T>
{
    return item;
}

template <class Dim0, class T>
constexpr auto operator-(const array_t<shape_t<Dim0>, T>& item) -> array_t<shape_t<Dim0>, T>
{
    auto out = array_t<shape_t<Dim0>, T>{ item.shape() };
    for (location_value_t i = 0; i < static_cast<location_value_t>(item.volume()); ++i)
    {
        out[i] = -item[i];
    }
    return out;
}

template <class Dim0, class L, class R, class Res = std::invoke_result_t<std::plus<>, L, R>>
constexpr auto operator+=(array_t<shape_t<Dim0>, L>& lhs, const array_t<shape_t<Dim0>, R>& rhs) -> array_t<shape_t<Dim0>, L>&
{
    for (location_value_t i = 0; i < static_cast<location_value_t>(lhs.volume()); ++i)
    {
        lhs[i] = std::plus<>{}(lhs[i], rhs[i]);
    }
    return lhs;
}

template <class Dim0, class L, class R, class Res = std::invoke_result_t<std::plus<>, L, R>>
constexpr auto operator+(const array_t<shape_t<Dim0>, L>& lhs, const array_t<shape_t<Dim0>, R>& rhs)
    -> array_t<shape_t<Dim0>, Res>
{
    auto out = array_t<shape_t<Dim0>, Res>{ lhs.shape() };
    for (location_value_t i = 0; i < static_cast<location_value_t>(lhs.volume()); ++i)
    {
        out[i] = std::plus<>{}(lhs[i], rhs[i]);
    }
    return out;
}

template <class Dim0, class L, class R, class Res = std::invoke_result_t<std::minus<>, L, R>>
constexpr auto operator-=(array_t<shape_t<Dim0>, L>& lhs, const array_t<shape_t<Dim0>, R>& rhs) -> array_t<shape_t<Dim0>, L>&
{
    for (location_value_t i = 0; i < static_cast<location_value_t>(lhs.volume()); ++i)
    {
        lhs[i] = std::minus<>{}(lhs[i], rhs[i]);
    }
    return lhs;
}

template <class Dim0, class L, class R, class Res = std::invoke_result_t<std::minus<>, L, R>>
constexpr auto operator-(const array_t<shape_t<Dim0>, L>& lhs, const array_t<shape_t<Dim0>, R>& rhs)
    -> array_t<shape_t<Dim0>, Res>
{
    auto out = array_t<shape_t<Dim0>, Res>{ lhs.shape() };
    for (location_value_t i = 0; i < static_cast<location_value_t>(lhs.volume()); ++i)
    {
        out[i] = std::minus<>{}(lhs[i], rhs[i]);
    }
    return out;
}

template <
    class Dim0,
    class L,
    class R,
    std::enable_if_t<is_scalar_t<R>::value && std::is_invocable_v<std::multiplies<>, L, R>, int> = 0,
    class Res = std::invoke_result_t<std::multiplies<>, L, R>>
constexpr auto operator*=(array_t<shape_t<Dim0>, L>& lhs, R rhs) -> array_t<shape_t<Dim0>, L>&
{
    for (location_value_t i = 0; i < static_cast<location_value_t>(lhs.volume()); ++i)
    {
        lhs[i] = std::multiplies<>{}(lhs[i], rhs);
    }
    return lhs;
}

template <
    class Dim0,
    class L,
    class R,
    std::enable_if_t<is_scalar_t<R>::value && std::is_invocable_v<std::multiplies<>, L, R>, int> = 0,
    class Res = std::invoke_result_t<std::multiplies<>, L, R>>
constexpr auto operator*(const array_t<shape_t<Dim0>, L>& lhs, R rhs) -> array_t<shape_t<Dim0>, Res>
{
    auto out = array_t<shape_t<Dim0>, Res>{ lhs.shape() };
    for (location_value_t i = 0; i < static_cast<location_value_t>(lhs.volume()); ++i)
    {
        out[i] = std::multiplies<>{}(lhs[i], rhs);
    }
    return out;
}

template <
    class L,
    class Dim0,
    class R,
    std::enable_if_t<is_scalar_t<L>::value && std::is_invocable_v<std::multiplies<>, L, R>, int> = 0,
    class Res = std::invoke_result_t<std::multiplies<>, L, R>>
constexpr auto operator*(L lhs, const array_t<shape_t<Dim0>, R>& rhs) -> array_t<shape_t<Dim0>, Res>
{
    return rhs * lhs;
}

template <
    class Dim0,
    class L,
    class R,
    std::enable_if_t<is_scalar_t<R>::value && std::is_invocable_v<std::divides<>, L, R>, int> = 0,
    class Res = std::invoke_result_t<std::divides<>, L, R>>
constexpr auto operator/=(array_t<shape_t<Dim0>, L>& lhs, R rhs) -> array_t<shape_t<Dim0>, L>&
{
    for (location_value_t i = 0; i < static_cast<location_value_t>(lhs.volume()); ++i)
    {
        lhs[i] = std::divides<>{}(lhs[i], rhs);
    }
    return lhs;
}

template <
    class Dim0,
    class L,
    class R,
    std::enable_if_t<is_scalar_t<R>::value && std::is_invocable_v<std::divides<>, L, R>, int> = 0,
    class Res = std::invoke_result_t<std::divides<>, L, R>>
constexpr auto operator/(const array_t<shape_t<Dim0>, L>& lhs, R rhs) -> array_t<shape_t<Dim0>, Res>
{
    auto out = array_t<shape_t<Dim0>, Res>{ lhs.shape() };
    for (location_value_t i = 0; i < static_cast<location_value_t>(lhs.volume()); ++i)
    {
        out[i] = std::divides<>{}(lhs[i], rhs);
    }
    return out;
}

template <class T>
struct dense_vector_traits
{
    static constexpr std::size_t dims = 0;
    using value_type = void;
};

template <extent_value_t E, stride_value_t S, class T>
struct dense_vector_traits<array_t<shape_t<dim_t<E, S>>, T>>
{
    static constexpr std::size_t dims = static_cast<std::size_t>(E);
    static constexpr stride_value_t stride = S;
    using value_type = T;
};

template <class T>
struct dense_matrix_traits
{
    static constexpr std::size_t rows = 0;
    static constexpr std::size_t cols = 0;
    static constexpr stride_value_t row_stride = 0;
    static constexpr stride_value_t col_stride = 0;
    using value_type = void;
};

template <extent_value_t R, stride_value_t RS, extent_value_t C, stride_value_t CS, class T>
struct dense_matrix_traits<array_t<shape_t<dim_t<R, RS>, dim_t<C, CS>>, T>>
{
    static constexpr std::size_t rows = static_cast<std::size_t>(R);
    static constexpr std::size_t cols = static_cast<std::size_t>(C);
    static constexpr stride_value_t row_stride = RS;
    static constexpr stride_value_t col_stride = CS;
    using value_type = T;
};

template <
    class Vec,
    class Mat,
    std::enable_if_t<
        is_dense_vector<std::decay_t<Vec>>::value && is_dense_matrix<std::decay_t<Mat>>::value
            && (dense_vector_traits<std::decay_t<Vec>>::dims + 1 == dense_matrix_traits<std::decay_t<Mat>>::rows)
            && (dense_matrix_traits<std::decay_t<Mat>>::rows == dense_matrix_traits<std::decay_t<Mat>>::cols)
            && (dense_vector_traits<std::decay_t<Vec>>::stride
                == static_cast<stride_value_t>(sizeof(typename dense_vector_traits<std::decay_t<Vec>>::value_type)))
            && (dense_matrix_traits<std::decay_t<Mat>>::col_stride
                == static_cast<stride_value_t>(sizeof(typename dense_matrix_traits<std::decay_t<Mat>>::value_type)))
            && (dense_matrix_traits<std::decay_t<Mat>>::row_stride
                == static_cast<stride_value_t>(
                    sizeof(typename dense_matrix_traits<std::decay_t<Mat>>::value_type)
                    * dense_matrix_traits<std::decay_t<Mat>>::cols)),
        int> = 0,
    class Res = std::invoke_result_t<
        std::multiplies<>,
        typename dense_vector_traits<std::decay_t<Vec>>::value_type,
        typename dense_matrix_traits<std::decay_t<Mat>>::value_type>>
constexpr auto operator*(const Vec& lhs, const Mat& rhs) -> dense_vector_t<dense_vector_traits<std::decay_t<Vec>>::dims, Res>
{
    constexpr std::size_t D = dense_vector_traits<std::decay_t<Vec>>::dims;
    dense_vector_t<D, Res> result{};

    for (std::size_t column = 0; column < D; ++column)
    {
        const auto loc_column = static_cast<location_value_t>(column);
        Res sum = static_cast<Res>(rhs[{ static_cast<location_value_t>(D), loc_column }]);
        for (std::size_t row = 0; row < D; ++row)
        {
            const auto loc_row = static_cast<location_value_t>(row);
            sum += lhs[loc_row] * rhs[{ loc_row, loc_column }];
        }
        result[loc_column] = sum;
    }

    return result;
}

template <
    class Lhs,
    class Rhs,
    std::enable_if_t<
        is_dense_matrix<std::decay_t<Lhs>>::value && is_dense_matrix<std::decay_t<Rhs>>::value
            && (dense_matrix_traits<std::decay_t<Lhs>>::cols == dense_matrix_traits<std::decay_t<Rhs>>::rows)
            && (dense_matrix_traits<std::decay_t<Lhs>>::col_stride
                == static_cast<stride_value_t>(sizeof(typename dense_matrix_traits<std::decay_t<Lhs>>::value_type)))
            && (dense_matrix_traits<std::decay_t<Rhs>>::col_stride
                == static_cast<stride_value_t>(sizeof(typename dense_matrix_traits<std::decay_t<Rhs>>::value_type)))
            && (dense_matrix_traits<std::decay_t<Lhs>>::row_stride
                == static_cast<stride_value_t>(
                    sizeof(typename dense_matrix_traits<std::decay_t<Lhs>>::value_type)
                    * dense_matrix_traits<std::decay_t<Lhs>>::cols))
            && (dense_matrix_traits<std::decay_t<Rhs>>::row_stride
                == static_cast<stride_value_t>(
                    sizeof(typename dense_matrix_traits<std::decay_t<Rhs>>::value_type)
                    * dense_matrix_traits<std::decay_t<Rhs>>::cols)),
        int> = 0,
    class Res = std::invoke_result_t<
        std::multiplies<>,
        typename dense_matrix_traits<std::decay_t<Lhs>>::value_type,
        typename dense_matrix_traits<std::decay_t<Rhs>>::value_type>>
constexpr auto operator*(const Lhs& lhs, const Rhs& rhs)
    -> dense_matrix_t<dense_matrix_traits<std::decay_t<Lhs>>::rows, dense_matrix_traits<std::decay_t<Rhs>>::cols, Res>
{
    constexpr std::size_t R = dense_matrix_traits<std::decay_t<Lhs>>::rows;
    constexpr std::size_t D = dense_matrix_traits<std::decay_t<Lhs>>::cols;
    constexpr std::size_t C = dense_matrix_traits<std::decay_t<Rhs>>::cols;

    dense_matrix_t<R, C, Res> result{};

    for (std::size_t row = 0; row < R; ++row)
    {
        const auto loc_row = static_cast<location_value_t>(row);
        for (std::size_t column = 0; column < C; ++column)
        {
            const auto loc_column = static_cast<location_value_t>(column);
            Res sum{};
            for (std::size_t inner = 0; inner < D; ++inner)
            {
                const auto loc_inner = static_cast<location_value_t>(inner);
                sum += lhs[{ loc_row, loc_inner }] * rhs[{ loc_inner, loc_column }];
            }
            result[{ loc_row, loc_column }] = sum;
        }
    }

    return result;
}

}  // namespace mat2
}  // namespace zx

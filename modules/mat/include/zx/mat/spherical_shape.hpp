#pragma once

#include <zx/mat/matrix.hpp>

namespace zx
{
namespace mat
{

template <class T, std::size_t D>
struct spherical_shape_t
{
    vector_t<T, D> center;
    T radius;
};

template <class T>
std::ostream& operator<<(std::ostream& os, const spherical_shape_t<T, 2>& item)
{
    return os << "(circle " << item.center << " " << item.radius << ")";
}

template <class T>
std::ostream& operator<<(std::ostream& os, const spherical_shape_t<T, 3>& item)
{
    return os << "(sphere " << item.center << " " << item.radius << ")";
}

template <class T, class U, std::size_t D>
constexpr auto operator+=(spherical_shape_t<T, D>& lhs, const vector_t<U, D>& rhs) -> spherical_shape_t<T, D>&
{
    lhs.center += rhs;
    return lhs;
}

template <class T, class U, std::size_t D>
constexpr auto operator+(spherical_shape_t<T, D> lhs, const vector_t<U, D>& rhs) -> spherical_shape_t<T, D>
{
    lhs.center += rhs;
    return lhs;
}

template <class T, class U, std::size_t D>
constexpr auto operator-=(spherical_shape_t<T, D>& lhs, const vector_t<U, D>& rhs) -> spherical_shape_t<T, D>&
{
    lhs.center -= rhs;
    return lhs;
}

template <class T, class U, std::size_t D>
constexpr auto operator-(spherical_shape_t<T, D> lhs, const vector_t<U, D>& rhs) -> spherical_shape_t<T, D>
{
    lhs.center -= rhs;
    return lhs;
}

}  // namespace mat

}  // namespace zx

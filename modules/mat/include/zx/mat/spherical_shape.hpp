#pragma once

#include <zx/mat/matrix.hpp>

namespace zx
{
namespace mat
{

template <std::size_t D, class T>
struct spherical_shape_t
{
    vector_t<D, T> center;
    T radius;
};

template <std::size_t D, class T>
struct is_shape<spherical_shape_t<D, T>> : public std::true_type
{
};

template <class T>
std::ostream& operator<<(std::ostream& os, const spherical_shape_t<2, T>& item)
{
    return os << "(circle " << item.center << " " << item.radius << ")";
}

template <class T>
std::ostream& operator<<(std::ostream& os, const spherical_shape_t<3, T>& item)
{
    return os << "(sphere " << item.center << " " << item.radius << ")";
}

template <class T, class U, std::size_t D>
constexpr auto operator+=(spherical_shape_t<D, T>& lhs, const vector_t<D, U>& rhs) -> spherical_shape_t<D, T>&
{
    lhs.center += rhs;
    return lhs;
}

template <class T, class U, std::size_t D>
constexpr auto operator+(spherical_shape_t<D, T> lhs, const vector_t<D, U>& rhs) -> spherical_shape_t<D, T>
{
    lhs.center += rhs;
    return lhs;
}

template <class T, class U, std::size_t D>
constexpr auto operator-=(spherical_shape_t<D, T>& lhs, const vector_t<D, U>& rhs) -> spherical_shape_t<D, T>&
{
    lhs.center -= rhs;
    return lhs;
}

template <class T, class U, std::size_t D>
constexpr auto operator-(spherical_shape_t<D, T> lhs, const vector_t<D, U>& rhs) -> spherical_shape_t<D, T>
{
    lhs.center -= rhs;
    return lhs;
}

}  // namespace mat

}  // namespace zx

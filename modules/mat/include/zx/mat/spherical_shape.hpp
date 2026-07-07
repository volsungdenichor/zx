#pragma once

#include <zx/mat/matrix.hpp>

namespace zx
{
namespace mat
{

template <std::size_t D, class T>
struct spherical_shape_t
{
    point_t<D, T> center;
    T radius;
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

}  // namespace mat

}  // namespace zx

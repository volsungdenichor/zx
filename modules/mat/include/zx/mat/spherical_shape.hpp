#pragma once

#include <zx/mat/matrix.hpp>

namespace zx
{
namespace mat
{

template <std::size_t D, class T, class Space = cartesian_space_t>
struct spherical_shape_t
{
    point_t<D, T, Space> center;
    T radius;

    friend std::ostream& operator<<(std::ostream& os, const spherical_shape_t& item)
    {
        return os << "[" << item.center << " " << item.radius << "]";
    }
};

template <class T, class Space = cartesian_space_t>
using circle_t = spherical_shape_t<2, T, Space>;

template <class T, class Space = cartesian_space_t>
using sphere_t = spherical_shape_t<3, T, Space>;

}  // namespace mat

}  // namespace zx

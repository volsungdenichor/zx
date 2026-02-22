#pragma once

#include <array>
#include <cstdint>
#include <ostream>

namespace zx
{

namespace images
{

using byte_t = std::uint8_t;

struct rgb_color_t : public std::array<byte_t, 3>
{
    using base_t = std::array<byte_t, 3>;
    using base_t::base_t;

    rgb_color_t(byte_t r, byte_t g, byte_t b) : base_t{ r, g, b } { }

    friend std::ostream& operator<<(std::ostream& os, const rgb_color_t& item)
    {
        return os << "rgb(" << static_cast<int>(item[0]) << " " << static_cast<int>(item[1]) << " "
                  << static_cast<int>(item[2]) << ")";
    }
};

}  // namespace images
}  // namespace zx
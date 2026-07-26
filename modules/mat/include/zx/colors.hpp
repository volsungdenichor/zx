#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <ostream>
#include <vector>
#include <zx/function_ref.hpp>

namespace zx
{

namespace mat
{

using byte_t = std::uint8_t;

template <class T>
struct rgb_color_base_t
{
};

template <>
struct rgb_color_base_t<float> : std::array<float, 3>
{
    using base_t = std::array<float, 3>;
    using base_t::base_t;

    rgb_color_base_t(float r, float g, float b) : base_t{ r, g, b } { }

    friend rgb_color_base_t& operator*=(rgb_color_base_t& lhs, float rhs)
    {
        lhs[0] *= rhs;
        lhs[1] *= rhs;
        lhs[2] *= rhs;
        return lhs;
    }

    friend rgb_color_base_t operator*(rgb_color_base_t lhs, float rhs) { return lhs *= rhs; }

    friend rgb_color_base_t operator*(float lhs, rgb_color_base_t rhs) { return rhs *= lhs; }

    friend rgb_color_base_t& operator/=(rgb_color_base_t& lhs, float rhs)
    {
        lhs[0] /= rhs;
        lhs[1] /= rhs;
        lhs[2] /= rhs;
        return lhs;
    }

    friend rgb_color_base_t operator/(rgb_color_base_t lhs, float rhs) { return lhs /= rhs; }

    friend rgb_color_base_t& operator+=(rgb_color_base_t& lhs, const rgb_color_base_t& rhs)
    {
        lhs[0] += rhs[0];
        lhs[1] += rhs[1];
        lhs[2] += rhs[2];
        return lhs;
    }

    friend rgb_color_base_t operator+(rgb_color_base_t lhs, const rgb_color_base_t& rhs) { return lhs += rhs; }

    friend rgb_color_base_t& operator-=(rgb_color_base_t& lhs, const rgb_color_base_t& rhs)
    {
        lhs[0] -= rhs[0];
        lhs[1] -= rhs[1];
        lhs[2] -= rhs[2];
        return lhs;
    }

    friend rgb_color_base_t operator-(rgb_color_base_t lhs, const rgb_color_base_t& rhs) { return lhs -= rhs; }
};

template <>
struct rgb_color_base_t<byte_t> : std::array<byte_t, 3>
{
    using base_t = std::array<byte_t, 3>;
    using base_t::base_t;

    rgb_color_base_t(byte_t r, byte_t g, byte_t b) : base_t{ r, g, b } { }

    rgb_color_base_t(const rgb_color_base_t<float>& color)
        : base_t{ from_float(color[0]), from_float(color[1]), from_float(color[2]) }
    {
    }

    operator rgb_color_base_t<float>() const
    {
        return rgb_color_base_t<float>{ to_float((*this)[0]), to_float((*this)[1]), to_float((*this)[2]) };
    }

    static byte_t from_float(float value) { return static_cast<byte_t>(std::clamp(value, 0.F, 255.F)); }

    static float to_float(byte_t value) { return static_cast<float>(value); }

    friend std::ostream& operator<<(std::ostream& os, const rgb_color_base_t& item)
    {
        return os << "(rgb " << static_cast<int>(item[0]) << " " << static_cast<int>(item[1]) << " "
                  << static_cast<int>(item[2]) << ")";
    }

    friend bool operator==(const rgb_color_base_t& lhs, const rgb_color_base_t& rhs)
    {
        return lhs[0] == rhs[0] && lhs[1] == rhs[1] && lhs[2] == rhs[2];
    }

    friend bool operator!=(const rgb_color_base_t& lhs, const rgb_color_base_t& rhs) { return !(lhs == rhs); }
};

using true_color_t = rgb_color_base_t<byte_t>;
using rgb_color_t = rgb_color_base_t<float>;

struct lookup_table_t
{
    std::vector<float> m_table;

    lookup_table_t(std::vector<float> table) : m_table(std::move(table))
    {
        if (m_table.size() != 256)
        {
            throw std::invalid_argument("lookup table must have 256 entries");
        }
    }

    static lookup_table_t create(function_ref<float(float)> func)
    {
        std::vector<float> result(256);

        for (std::size_t i = 0; i < result.size(); ++i)
        {
            result[i] = func(static_cast<float>(i));
        }

        return lookup_table_t(std::move(result));
    }

    float operator()(byte_t value) const { return m_table[value]; }

    rgb_color_t operator()(const true_color_t& color) const
    {
        return rgb_color_t{ (*this)(color[0]), (*this)(color[1]), (*this)(color[2]) };
    }

    friend lookup_table_t operator*(const lookup_table_t& lhs, const lookup_table_t& rhs)
    {
        return lookup_table_t::create([&](float v) { return rhs(true_color_t::from_float(lhs(static_cast<byte_t>(v)))); });
    }
};

struct lookup_table
{
    static lookup_table_t identity()
    {
        return lookup_table_t::create([=](float v) { return v; });
    }

    static lookup_table_t brightness(float value)
    {
        return lookup_table_t::create([=](float v) { return v + value; });
    }

    static lookup_table_t threshold(float value, float lower, float upper)
    {
        return lookup_table_t::create([=](float v) { return v < value ? lower : upper; });
    }

    static lookup_table_t threshold(float value) { return threshold(value, 0.F, 255.F); }

    static lookup_table_t negative()
    {
        return lookup_table_t::create([](float v) { return 255.F - v; });
    }

    static lookup_table_t contrast(float value, float center = 128.F)
    {
        return lookup_table_t::create([=](float v) { return value * (v - center) + center; });
    }

    static lookup_table_t exposition(float value)
    {
        return value < 0 ? identity() : lookup_table_t::create([=](float v) { return v * value; });
    }

    static lookup_table_t levels_adjustment(const interval_t<float>& in, const interval_t<float>& out, float gamma = 1.F)
    {
        return lookup_table_t::create(
            [=](float v)
            {
                if (v < in[0])
                {
                    return out[0];
                }

                if (v > in[1])
                {
                    return out[1];
                }

                return out[0] + (out[1] - out[0]) * std::pow(float(v - in[0]) / (in[1] - in[0]), 1.F / gamma);
            });
    }

    static lookup_table_t gamma(float value) { return levels_adjustment({ 0.F, 255.F }, { 0.F, 255.F }, value); }
};

namespace filters
{

inline auto blend(float alpha)
{
    return [=](const rgb_color_t& dst, const rgb_color_t& src)
    {
        const auto apply = [=](float d, float s) { return d * (1.F - alpha) + s * alpha; };
        return rgb_color_t{ apply(dst[0], src[0]), apply(dst[1], src[1]), apply(dst[2], src[2]) };
    };
}

inline auto normal()
{
    return [](const rgb_color_t&, const rgb_color_t& src) { return src; };
}

inline auto lighter()
{
    return [](const rgb_color_t& dst, const rgb_color_t& src)
    {
        const auto apply = [](float d, float s) { return std::max(d, s); };
        return rgb_color_t{ apply(dst[0], src[0]), apply(dst[1], src[1]), apply(dst[2], src[2]) };
    };
}

inline auto darker()
{
    return [](const rgb_color_t& dst, const rgb_color_t& src)
    {
        const auto apply = [](float d, float s) { return std::min(d, s); };
        return rgb_color_t{ apply(dst[0], src[0]), apply(dst[1], src[1]), apply(dst[2], src[2]) };
    };
}

inline auto multiply()
{
    return [](const rgb_color_t& dst, const rgb_color_t& src)
    {
        const auto apply = [](float d, float s) { return d * s / 255.F; };
        return rgb_color_t{ apply(dst[0], src[0]), apply(dst[1], src[1]), apply(dst[2], src[2]) };
    };
}

inline auto screen()
{
    return [](const rgb_color_t& dst, const rgb_color_t& src)
    {
        const auto apply = [](float d, float s) { return 255.F - (255.F - d) * (255.F - s) / 255.F; };
        return rgb_color_t{ apply(dst[0], src[0]), apply(dst[1], src[1]), apply(dst[2], src[2]) };
    };
}

inline auto difference()
{
    return [](const rgb_color_t& dst, const rgb_color_t& src)
    {
        const auto apply = [](float d, float s) { return std::abs(d - s); };
        return rgb_color_t{ apply(dst[0], src[0]), apply(dst[1], src[1]), apply(dst[2], src[2]) };
    };
}

inline auto overlay()
{
    return [](const rgb_color_t& dst, const rgb_color_t& src)
    {
        const auto apply = [](float d, float s)
        { return d < 128.F ? (2.F * d * s / 255.F) : (255.F - 2.F * (255.F - d) * (255.F - s) / 255.F); };

        return rgb_color_t{ apply(dst[0], src[0]), apply(dst[1], src[1]), apply(dst[2], src[2]) };
    };
}

inline auto add()
{
    return [](const rgb_color_t& dst, const rgb_color_t& src)
    {
        const auto apply = [](float d, float s) { return s + d; };
        return rgb_color_t{ apply(dst[0], src[0]), apply(dst[1], src[1]), apply(dst[2], src[2]) };
    };
}

inline auto subtract()
{
    return [](const rgb_color_t& dst, const rgb_color_t& src)
    {
        const auto apply = [](float d, float s) { return d + s - 255.F; };
        return rgb_color_t{ apply(dst[0], src[0]), apply(dst[1], src[1]), apply(dst[2], src[2]) };
    };
}

inline auto sepia()
{
    return [](const rgb_color_t& color)
    {
        static const std::array<std::array<float, 3>, 3> coeffs
            = { { { 0.393F, 0.769F, 0.189F }, { 0.349F, 0.686F, 0.168F }, { 0.272F, 0.534F, 0.131F } } };

        rgb_color_t result;
        for (std::size_t i = 0; i < 3; ++i)
        {
            result[i] = std::inner_product(coeffs[i].begin(), coeffs[i].end(), color.begin(), 0.F);
        }
        return result;
    };
}

inline auto gray()
{
    return [](const rgb_color_t& color)
    {
        static const std::array<float, 3> coeffs = { 0.299F, 0.587F, 0.114F };

        const float v = std::inner_product(coeffs.begin(), coeffs.end(), color.begin(), 0.F);
        return rgb_color_t{ v, v, v };
    };
}

inline auto solid(const rgb_color_t& new_color, float alpha = 1.F)
{
    return [=](const rgb_color_t& color) { return blend(alpha)(color, new_color); };
}

}  // namespace filters

}  // namespace mat
}  // namespace zx

#pragma once

#include <cstdint>
#include <fstream>
#include <zx/array.hpp>
#include <zx/colors.hpp>

namespace zx
{
namespace images
{

using rgb_image_t = arrays::array_t<byte_t, 3>;

namespace detail
{

template <class T, class U = T>
void write(std::ostream& os, const U& value)
{
    os.write(reinterpret_cast<const char*>(&value), sizeof(T));
}

template <class T>
T read(std::istream& is)
{
    T value;
    is.read(reinterpret_cast<char*>(&value), sizeof(T));
    return value;
}

inline void write_n(std::ostream& os, std::size_t count, byte_t value = 0)
{
    for (std::size_t i = 0; i < count; ++i)
    {
        write<byte_t>(os, value);
    }
}

inline auto get_padding(std::size_t width, std::size_t bits_per_pixel) -> std::size_t
{
    return ((bits_per_pixel * width + 31) / 32) * 4 - (width * bits_per_pixel / 8);
}

struct bmp_header
{
    static const inline std::size_t size = 14;

    std::size_t file_size;
    std::size_t data_offset;

    bmp_header() : file_size(0), data_offset(0) { }

    void save(std::ostream& os) const
    {
        write<byte_t>(os, 'B');
        write<byte_t>(os, 'M');
        write<std::uint32_t>(os, file_size);
        write<std::uint16_t>(os, 0); /* reserved1 */
        write<std::uint16_t>(os, 0); /* reserved2 */
        write<std::uint32_t>(os, data_offset);
    }

    static auto load(std::istream& is) -> bmp_header
    {
        bmp_header result = {};
        read<byte_t>(is);                             /* B */
        read<byte_t>(is);                             /* M */
        result.file_size = read<std::uint32_t>(is);   /**/
        read<std::uint16_t>(is);                      /* reserved1 */
        read<std::uint16_t>(is);                      /* reserved2 */
        result.data_offset = read<std::uint32_t>(is); /* data_offset */
        return result;
    }
};

struct dib_header
{
    static const inline std::size_t size = 40;

    dib_header()
        : width(0)
        , height(0)
        , color_plane_count(0)
        , bits_per_pixel(0)
        , compression(0)
        , data_size(0)
        , horizontal_pixel_per_meter(0)
        , vertical_pixel_per_meter(0)
        , color_count(0)
        , important_color_count(0)
    {
    }

    std::size_t width;
    std::size_t height;
    std::size_t color_plane_count;
    std::size_t bits_per_pixel;
    std::size_t compression;
    std::size_t data_size;
    std::size_t horizontal_pixel_per_meter;
    std::size_t vertical_pixel_per_meter;
    std::size_t color_count;
    std::size_t important_color_count;

    void save(std::ostream& os) const
    {
        write<std::uint32_t>(os, size);
        write<std::uint32_t>(os, width);
        write<std::uint32_t>(os, height);
        write<std::uint16_t>(os, color_plane_count);
        write<std::uint16_t>(os, bits_per_pixel);
        write<std::uint32_t>(os, compression);
        write<std::uint32_t>(os, data_size);
        write<std::uint32_t>(os, horizontal_pixel_per_meter);
        write<std::uint32_t>(os, vertical_pixel_per_meter);
        write<std::uint32_t>(os, color_count);
        write<std::uint32_t>(os, important_color_count);
    }

    static auto load(std::istream& is) -> dib_header
    {
        dib_header result = {};
        read<std::uint32_t>(is); /* size */
        result.width = read<std::uint32_t>(is);
        result.height = read<std::uint32_t>(is);
        result.color_plane_count = read<std::uint16_t>(is);
        result.bits_per_pixel = read<std::uint16_t>(is);
        result.compression = read<std::uint32_t>(is);
        result.data_size = read<std::uint32_t>(is);
        result.horizontal_pixel_per_meter = read<std::uint32_t>(is);
        result.vertical_pixel_per_meter = read<std::uint32_t>(is);
        result.color_count = read<std::uint32_t>(is);
        result.important_color_count = read<std::uint32_t>(is);
        return result;
    }
};

inline void save_header(
    std::ostream& os,  //
    std::size_t width,
    std::size_t height,
    std::size_t padding,
    std::size_t bits_per_pixel,
    std::size_t palette_size)
{
    const std::size_t data_size = (width + padding) * height * (bits_per_pixel / 8);
    const std::size_t data_offset = bmp_header::size + dib_header::size + palette_size;
    const std::size_t file_size = data_offset + data_size;

    bmp_header bmp_hdr = {};
    bmp_hdr.file_size = file_size;
    bmp_hdr.data_offset = data_offset;

    dib_header dib_hdr = {};
    dib_hdr.width = width;
    dib_hdr.height = height;
    dib_hdr.color_plane_count = 1;
    dib_hdr.bits_per_pixel = bits_per_pixel;
    dib_hdr.compression = 0;
    dib_hdr.data_size = data_size;

    bmp_hdr.save(os);
    dib_hdr.save(os);
}

struct load_bitmap_fn
{
    auto operator()(std::istream& is) const -> rgb_image_t
    {
        if (!is)
        {
            throw std::runtime_error{ "load_bitmap: invalid stream" };
        }

        const bmp_header bmp_hdr = bmp_header::load(is);
        const dib_header dib_hdr = dib_header::load(is);

        (void)bmp_hdr;

        switch (dib_hdr.bits_per_pixel)
        {
            case 8: return load_bitmap_8(is, dib_hdr);
            case 24: return load_bitmap_24(is, dib_hdr);
            default: throw std::runtime_error{ "load_bitmap: format not supported" };
        }
    }

    auto operator()(const std::string& path) const -> rgb_image_t
    {
        std::ifstream fs(path.c_str(), std::ifstream::binary);
        if (!fs)
        {
            throw std::runtime_error{ std::string("load_bitmap: can not load file '" + path + "'") };
        }
        return (*this)(fs);
    }

    static auto prepare_array(const dib_header& header) -> rgb_image_t
    {
        return rgb_image_t{ rgb_image_t::size_type{
            static_cast<zx::arrays::size_base_t>(header.height), static_cast<zx::arrays::size_base_t>(header.width), 3 } };
    }

    static auto load_bitmap_8(std::istream& is, const dib_header& header) -> rgb_image_t
    {
        const auto padding = get_padding(header.width, header.bits_per_pixel);

        rgb_image_t result = prepare_array(header);
        auto ref = result.mut_view();

        using palette_t = std::array<zx::images::rgb_color_t, 256>;
        palette_t palette = {};

        for (std::size_t i = 0; i < 256; ++i)
        {
            const byte_t b = read<byte_t>(is);
            const byte_t g = read<byte_t>(is);
            const byte_t r = read<byte_t>(is);
            is.ignore(1);

            palette[i] = zx::images::rgb_color_t{ r, g, b };
        }

        const zx::arrays::size_base_t h = ref.shape().m_dims[0].size;
        const zx::arrays::size_base_t w = ref.shape().m_dims[1].size;

        for (zx::arrays::location_base_t y = h - 1; y >= 0; --y)
        {
            for (zx::arrays::location_base_t x = 0; x < w; ++x)
            {
                const zx::images::rgb_color_t rgb = palette.at(read<byte_t>(is));
                for (std::size_t z = 0; z < 3; ++z)
                {
                    ref[rgb_image_t::location_type{ y, x, z }] = rgb[z];
                }
            }

            is.ignore(static_cast<std::streamsize>(padding));
        }

        return result;
    }

    static auto load_bitmap_24(std::istream& is, const dib_header& header) -> rgb_image_t
    {
        const auto padding = get_padding(header.width, header.bits_per_pixel);

        rgb_image_t result = prepare_array(header);
        auto ref = result.mut_view();

        const zx::arrays::size_base_t h = ref.shape().m_dims[0].size;
        const zx::arrays::size_base_t w = ref.shape().m_dims[1].size;

        for (zx::arrays::location_base_t y = h - 1; y >= 0; --y)
        {
            for (zx::arrays::location_base_t x = 0; x < w; ++x)
            {
                for (zx::arrays::location_base_t z = 2; z >= 0; --z)
                {
                    const byte_t value = read<byte_t>(is);
                    ref[rgb_image_t::location_type{ y, x, z }] = value;
                }
            }

            is.ignore(static_cast<std::streamsize>(padding));
        }
        return result;
    }
};

struct save_bitmap_fn
{
    void operator()(rgb_image_t::view_type image, std::ostream& os) const
    {
        static const std::size_t bits_per_pixel = 24;
        const std::size_t padding = get_padding(static_cast<std::size_t>(image.shape().m_dims[1].size), bits_per_pixel);

        const zx::arrays::size_base_t h = image.shape().m_dims[0].size;
        const zx::arrays::size_base_t w = image.shape().m_dims[1].size;

        save_header(os, static_cast<std::size_t>(w), static_cast<std::size_t>(h), padding, bits_per_pixel, 0);

        for (zx::arrays::location_base_t y = h - 1; y >= 0; --y)
        {
            for (zx::arrays::location_base_t x = 0; x < w; ++x)
            {
                for (zx::arrays::location_base_t z = 2; z >= 0; --z)
                {
                    write<byte_t>(os, image[rgb_image_t::location_type{ y, x, z }]);
                }
            }

            write_n(os, padding);
        }
    }

    void operator()(rgb_image_t::view_type image, const std::string& path) const
    {
        std::ofstream fs(path.c_str(), std::ofstream::binary);
        (*this)(image, fs);
    }
};

}  // namespace detail

static constexpr inline auto load_bitmap = detail::load_bitmap_fn{};
static constexpr inline auto save_bitmap = detail::save_bitmap_fn{};

inline zx::images::rgb_color_t at(rgb_image_t::view_type image, const zx::mat::vector<zx::arrays::location_base_t, 2>& loc)
{
    const auto r = image[rgb_image_t::location_type{ loc[0], loc[1], 0 }];
    const auto g = image[rgb_image_t::location_type{ loc[0], loc[1], 1 }];
    const auto b = image[rgb_image_t::location_type{ loc[0], loc[1], 2 }];
    return zx::images::rgb_color_t{ r, g, b };
}

inline void at(
    rgb_image_t::mut_view_type image,
    const zx::mat::vector<zx::arrays::location_base_t, 2>& loc,
    const zx::images::rgb_color_t& color)
{
    image[rgb_image_t::location_type{ loc[0], loc[1], 0 }] = color[0];
    image[rgb_image_t::location_type{ loc[0], loc[1], 1 }] = color[1];
    image[rgb_image_t::location_type{ loc[0], loc[1], 2 }] = color[2];
}

}  // namespace images

}  // namespace zx

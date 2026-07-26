#pragma once

#include <cstdint>
#include <fstream>
#include <zx/array.hpp>
#include <zx/colors.hpp>
#include <zx/format.hpp>
#include <zx/function_ref.hpp>
#include <zx/raster.hpp>

namespace zx
{
namespace mat
{

struct filepath_t
{
    std::string m_path;

    explicit filepath_t(const std::string& path) : m_path(path) { }

    const char* c_str() const { return m_path.c_str(); }

    friend bool operator==(const filepath_t& lhs, const filepath_t& rhs) { return lhs.m_path == rhs.m_path; }
    friend bool operator!=(const filepath_t& lhs, const filepath_t& rhs) { return !(lhs == rhs); }

    friend std::ostream& operator<<(std::ostream& os, const filepath_t& item) { return os << item.m_path; }
};

namespace detail
{
struct rgb_image_tag_t
{
};

}  // namespace detail

using rgb_image_t = array_t<byte_t, 3, detail::rgb_image_tag_t>;
using channel_t = array_t<byte_t, 2, detail::rgb_image_tag_t>;

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

    auto operator()(const filepath_t& path) const -> rgb_image_t
    {
        std::ifstream fs(path.c_str(), std::ifstream::binary);
        if (!fs)
        {
            throw std::runtime_error{ str("load_bitmap: can not load file '", path, "'") };
        }
        return (*this)(fs);
    }

    static auto prepare_array(const dib_header& header) -> rgb_image_t
    {
        return rgb_image_t{ rgb_image_t::extent_type{
            static_cast<extent_base_t>(header.height), static_cast<extent_base_t>(header.width), 3 } };
    }

    static auto load_bitmap_8(std::istream& is, const dib_header& header) -> rgb_image_t
    {
        const auto padding = get_padding(header.width, header.bits_per_pixel);

        rgb_image_t result = prepare_array(header);
        auto ref = result.mut_view();

        using palette_t = std::array<rgb_color_t, 256>;
        palette_t palette = {};

        for (std::size_t i = 0; i < 256; ++i)
        {
            palette[i][2] = read<byte_t>(is);
            palette[i][1] = read<byte_t>(is);
            palette[i][0] = read<byte_t>(is);
            is.ignore(1);
        }

        const extent_base_t h = ref.shape()[0].extent;
        const extent_base_t w = ref.shape()[1].extent;

        for (location_base_t y = h - 1; y >= 0; --y)
        {
            for (location_base_t x = 0; x < w; ++x)
            {
                const rgb_color_t rgb = palette.at(read<byte_t>(is));
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

        const extent_base_t h = ref.shape()[0].extent;
        const extent_base_t w = ref.shape()[1].extent;

        for (location_base_t y = h - 1; y >= 0; --y)
        {
            for (location_base_t x = 0; x < w; ++x)
            {
                for (location_base_t z = 2; z >= 0; --z)
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
        const std::size_t padding = get_padding(static_cast<std::size_t>(image.shape()[1].extent), bits_per_pixel);

        const extent_base_t h = image.shape()[0].extent;
        const extent_base_t w = image.shape()[1].extent;

        save_header(os, static_cast<std::size_t>(w), static_cast<std::size_t>(h), padding, bits_per_pixel, 0);

        for (location_base_t y = h - 1; y >= 0; --y)
        {
            for (location_base_t x = 0; x < w; ++x)
            {
                for (location_base_t z = 2; z >= 0; --z)
                {
                    write<byte_t>(os, image[rgb_image_t::location_type{ y, x, z }]);
                }
            }

            write_n(os, padding);
        }
    }

    void operator()(rgb_image_t::view_type image, const filepath_t& path) const
    {
        std::ofstream fs(path.c_str(), std::ofstream::binary);
        (*this)(image, fs);
    }
};

template <class T>
struct inject_t
{
    T m_value;

    constexpr inject_t(T value) : m_value{ std::move(value) } { }

    template <class... Args>
    constexpr const T& operator()(Args&&...) const
    {
        return m_value;
    }
};

using color_filter_t = function_ref<rgb_color_t(const rgb_color_t&)>;

struct at_fn
{
    rgb_color_t operator()(const rgb_image_t::view_type& image, const location_t<2>& loc) const
    {
        rgb_color_t result = {};
        for (std::size_t z = 0; z < 3; ++z)
        {
            result[z] = image[rgb_image_t::location_type{ loc[0], loc[1], z }];
        }
        return result;
    }

    void operator()(const rgb_image_t::mut_view_type& image, const location_t<2>& loc, const rgb_color_t& color) const
    {
        for (std::size_t z = 0; z < 3; ++z)
        {
            image[rgb_image_t::location_type{ loc[0], loc[1], z }] = color[z];
        }
    }
};

static constexpr inline auto at = at_fn{};

struct modify
{
    void operator()(const rgb_image_t::mut_view_type& image, const location_t<2>& loc, color_filter_t filter) const
    {
        at(image, loc, filter(at(image, loc)));
    }

    void operator()(const rgb_image_t::mut_view_type& image, color_filter_t filter) const
    {
        const extent_base_t h = image.shape()[0].extent;
        const extent_base_t w = image.shape()[1].extent;

        for (location_base_t y = 0; y < h; ++y)
        {
            for (location_base_t x = 0; x < w; ++x)
            {
                (*this)(image, location_t<2>{ y, x }, filter);
            }
        }
    }
};

struct channel_fn
{
    static shape_t<2> get_channel_shape(const rgb_image_t::shape_type& shape) { return shape.erase(2); }

    channel_t::view_type operator()(const rgb_image_t::view_type& image, std::size_t channel_index) const
    {
        return channel_t::view_type{ image.data() + channel_index, get_channel_shape(image.shape()) };
    }

    inline channel_t::mut_view_type operator()(const rgb_image_t::mut_view_type& image, std::size_t channel_index) const
    {
        return channel_t::mut_view_type{ image.data() + channel_index, get_channel_shape(image.shape()) };
    }
};

struct rotate_fn
{
    template <class T, class Tag>
    auto operator()(array_view_base_t<T, 2, Tag> image, int degrees) const -> array_view_base_t<T, 2, Tag>
    {
        const auto [shape, offset] = new_shape_and_offset(image.shape(), normalize_quarter_turns(degrees));
        return { image.from_offset(offset), shape };
    }

    template <class T, class Tag>
    auto operator()(array_view_base_t<T, 3, Tag> image, int degrees) const -> array_view_base_t<T, 3, Tag>
    {
        const auto [shape, offset] = new_shape_and_offset(image.shape(), normalize_quarter_turns(degrees));
        return { image.from_offset(offset), shape };
    }

    static int normalize_quarter_turns(int degrees)
    {
        if (degrees % 90 != 0)
        {
            throw std::invalid_argument{ "rotate: only multiples of 90 degrees are supported" };
        }

        const int turns = degrees / 90;
        return (turns % 4 + 4) % 4;
    }

    static auto new_shape_and_offset(const shape_t<2>& src, int turns) -> std::pair<shape_t<2>, flat_offset_t>
    {
        if (src[0].extent == 0 || src[1].extent == 0 || turns == 0)
        {
            return { src, 0 };
        }

        const auto bounds = src.bounds();
        switch (turns)
        {
            case 0: return { src, 0 };
            case 1:
                return std::pair{ shape_t<2>{ src[1], src[0].flip() },
                                  src.flat_offset(bounds.get({ side_t::last, side_t::first })) };

            case 2:
                return std::pair{ shape_t<2>{ src[0].flip(), src[1].flip() },
                                  src.flat_offset(bounds.get({ side_t::last, side_t::last })) };
            case 3:
                return std::pair{ shape_t<2>{ src[1].flip(), src[0] },
                                  src.flat_offset(bounds.get({ side_t::first, side_t::last })) };
        }
        return { src, 0 };
    }

    static auto new_shape_and_offset(const shape_t<3>& src, int turns) -> std::pair<shape_t<3>, flat_offset_t>
    {
        if (src[0].extent == 0 || src[1].extent == 0 || turns == 0)
        {
            return { src, 0 };
        }

        const auto bounds = src.bounds();
        switch (turns)
        {
            case 0: return { src, 0 };
            case 1:
                return std::pair{ shape_t<3>{ src[1], src[0].flip(), src[2] },
                                  src.flat_offset(bounds.get({ side_t::last, side_t::first, side_t::first })) };

            case 2:
                return std::pair{ shape_t<3>{ src[0].flip(), src[1].flip(), src[2] },
                                  src.flat_offset(bounds.get({ side_t::last, side_t::last, side_t::first })) };
            case 3:
                return std::pair{ shape_t<3>{ src[1].flip(), src[0], src[2] },
                                  src.flat_offset(bounds.get({ side_t::first, side_t::last, side_t::first })) };
        }
        return { src, 0 };
    }
};

template <std::size_t D>
struct flip_fn
{
    static_assert(D < 2, "flip: axis out of range");

    template <class T, class Tag>
    auto operator()(array_view_base_t<T, 2, Tag> image) const -> array_view_base_t<T, 2, Tag>
    {
        const auto [shape, offset] = new_shape_and_offset(image.shape());
        return { image.from_offset(offset), shape };
    }

    template <class T, class Tag>
    auto operator()(array_view_base_t<T, 3, Tag> image) const -> array_view_base_t<T, 3, Tag>
    {
        const auto [shape, offset] = new_shape_and_offset(image.shape());
        return { image.from_offset(offset), shape };
    }

    static auto new_shape_and_offset(const shape_t<2>& src) -> std::pair<shape_t<2>, flat_offset_t>
    {
        const auto bounds = src.bounds();
        shape_t<2> out_shape = src;
        out_shape[D] = src[D].flip();

        const flat_offset_t base_offset = D == 0 ? src.flat_offset(bounds.get({ side_t::last, side_t::first }))
                                                 : src.flat_offset(bounds.get({ side_t::first, side_t::last }));
        return { out_shape, base_offset };
    }

    static auto new_shape_and_offset(const shape_t<3>& src) -> std::pair<shape_t<3>, flat_offset_t>
    {
        const auto bounds = src.bounds();
        shape_t<3> out_shape = src;
        out_shape[D] = src[D].flip();

        const flat_offset_t base_offset = D == 0
                                              ? src.flat_offset(bounds.get({ side_t::last, side_t::first, side_t::first }))
                                              : src.flat_offset(bounds.get({ side_t::first, side_t::last, side_t::first }));
        return { out_shape, base_offset };
    }
};

struct draw_pixel_t
{
    rgb_image_t::mut_view_type m_image;
    color_filter_t m_color_filter;

    void operator()(const location_t<2>& loc) const
    {
        if (contains(m_image.bounds(), { loc[0], loc[1], 0 }))
        {
            at(m_image, loc, m_color_filter(at(m_image, loc)));
        }
    }
};

struct bresenham_line_fn
{
    void operator()(
        const rgb_image_t::mut_view_type& image, const segment_t<2, location_base_t>& seg, const rgb_color_t& color) const
    {
        (*this)(image, seg, inject_t{ color });
    }

    void operator()(
        const rgb_image_t::mut_view_type& image,
        const segment_t<2, location_base_t>& seg,
        const color_filter_t& color_filter) const
    {
        (*this)(seg[0], seg[1], draw_pixel_t{ image, color_filter });
    }

    void operator()(
        const location_t<2>& start, const location_t<2>& end, function_ref<void(const location_t<2>&)> output) const
    {
        const auto direction = end - start;

        vector_t<2, location_base_t> dist;
        vector_t<2, location_base_t> dir;

        std::transform(direction.begin(), direction.end(), dist.begin(), math::abs);
        std::transform(direction.begin(), direction.end(), dir.begin(), math::sign);

        bresenham_line_fn::bresenham(
            start,
            dir,
            dist,
            (dist[0] > dist[1] ? dist[0] : -dist[1]) / 2,
            [&](const location_t<2>& loc)
            {
                output(loc);
                return loc != end;
            });
    }

    static void bresenham(
        location_t<2> cur,
        const vector_t<2, location_base_t>& dir,
        const vector_t<2, location_base_t>& dist,
        int err,
        function_ref<bool(const location_t<2>&)> output)
    {
        while (true)
        {
            if (!output(cur))
            {
                break;
            }

            auto e = err;

            if (e > -dist[0])
            {
                err -= dist[1];
                cur[0] += dir[0];
            }

            if (e < dist[1])
            {
                err += dist[0];
                cur[1] += dir[1];
            }
        }
    }
};

struct bresenham_circle_fn
{
    void operator()(
        const rgb_image_t::mut_view_type& image, const circle_t<location_base_t>& circle, const rgb_color_t& color) const
    {
        (*this)(image, circle, inject_t{ color });
    }

    void operator()(
        const rgb_image_t::mut_view_type& image, const circle_t<location_base_t>& circle, color_filter_t color_filter) const
    {
        (*this)(circle.center, circle.radius, draw_pixel_t{ image, color_filter });
    }

    void operator()(const location_t<2>& center, int radius, function_ref<void(const location_t<2>&)> output) const
    {
        mat::vector_t<2, location_base_t> cur{ radius, 0 };
        int err = 0;

        while (cur[0] >= cur[1])
        {
            output(point(center[0] + cur[0], center[1] + cur[1]));
            output(point(center[0] + cur[1], center[1] + cur[0]));
            output(point(center[0] - cur[1], center[1] + cur[0]));
            output(point(center[0] - cur[0], center[1] + cur[1]));
            output(point(center[0] - cur[0], center[1] - cur[1]));
            output(point(center[0] - cur[1], center[1] - cur[0]));
            output(point(center[0] + cur[1], center[1] - cur[0]));
            output(point(center[0] + cur[0], center[1] - cur[1]));

            if (err <= 0)
            {
                cur[1] += 1;
                err += 2 * cur[1] + 1;
            }

            if (err > 0)
            {
                cur[0] -= 1;
                err -= 2 * cur[0] + 1;
            }
        }
    }
};

struct draw_rectangle_fn
{
    void operator()(
        const rgb_image_t::mut_view_type& image, const rectangle_t<location_base_t>& rect, const rgb_color_t& color) const
    {
        (*this)(image, rect, inject_t{ color });
    }

    void operator()(
        const rgb_image_t::mut_view_type& image, const rectangle_t<location_base_t>& rect, color_filter_t color_filter) const
    {
        for (const auto seg : segments(rect))
        {
            bresenham_line_fn{}(image, seg, color_filter);
        }
    }
};

struct draw_raster_fn
{
    void operator()(const rgb_image_t::mut_view_type& image, const raster_t& raster, const rgb_color_t& color) const
    {
        (*this)(image, raster, inject_t{ color });
    }

    void operator()(const rgb_image_t::mut_view_type& image, const raster_t& raster, color_filter_t color_filter) const
    {
        const auto do_draw = draw_pixel_t{ image, color_filter };
        for (const auto& shape : raster)
        {
            (*this)(shape, do_draw);
        }
    }

    void operator()(const raster_t::shape_t& shape, const draw_pixel_t& do_draw) const
    {
        for (const auto& [y, spans] : shape)
        {
            for (const auto& span : spans)
            {
                for (location_base_t x = span[0]; x < span[1]; ++x)
                {
                    do_draw(location_t<2>{ y, x });
                }
            }
        }
    }
};

}  // namespace detail

using detail::at;
static constexpr inline auto modify = detail::modify{};

static constexpr inline auto load_bitmap = detail::load_bitmap_fn{};
static constexpr inline auto save_bitmap = detail::save_bitmap_fn{};
static constexpr inline auto channel = detail::channel_fn{};
static constexpr inline auto rotate = detail::rotate_fn{};
static constexpr inline auto flip_horizontal = detail::flip_fn<1>{};
static constexpr inline auto flip_vertical = detail::flip_fn<0>{};

static constexpr inline auto draw_line = detail::bresenham_line_fn{};
static constexpr inline auto draw_circle = detail::bresenham_circle_fn{};
static constexpr inline auto draw_rectangle = detail::draw_rectangle_fn{};
static constexpr inline auto draw_raster = detail::draw_raster_fn{};

}  // namespace mat

}  // namespace zx

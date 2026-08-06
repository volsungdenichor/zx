#pragma once

#include <algorithm>
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

template <extent_base_t Channels>
struct image_base_t
{
    using storage_type = array_t<byte_t, 3>;
    using channel_type = array_t<byte_t, 2>;

    storage_type m_data;

    struct proxy_t
    {
        array_mut_view_t<byte_t, 1> _data;

        void operator=(const true_color_t& color) const
        {
            for (std::size_t z = 0; z < 3; ++z)
            {
                _data[static_cast<location_base_t>(z)] = color[z];
            }
        }

        operator true_color_t() const
        {
            true_color_t result = {};
            for (std::size_t z = 0; z < 3; ++z)
            {
                result[z] = _data[static_cast<location_base_t>(z)];
            }
            return result;
        }

        operator rgb_color_t() const { return static_cast<true_color_t>(*this); }

        friend bool operator==(const proxy_t& lhs, const true_color_t& rhs) { return static_cast<true_color_t>(lhs) == rhs; }

        friend bool operator!=(const proxy_t& lhs, const true_color_t& rhs) { return !(lhs == rhs); }

        friend bool operator==(const true_color_t& lhs, const proxy_t& rhs) { return rhs == lhs; }

        friend bool operator!=(const true_color_t& lhs, const proxy_t& rhs) { return rhs != lhs; }
    };

    struct view_type
    {
        using extent_type = extent_t<2, extent_base_t>;
        using location_type = location_t<2>;
        using bounds_type = bounds_t<2>;
        using slice_type = vector_t<2, slice_base_t>;

        using data_type = storage_type::view_type;
        data_type m_data;

        data_type data() const { return m_data; }
        extent_type extent() const { return pop_back(m_data.extent()); }

        true_color_t operator[](const location_type& loc) const
        {
            true_color_t result = {};
            for (std::size_t z = 0; z < 3; ++z)
            {
                result[z] = m_data[data_type::location_type{ loc[0], loc[1], z }];
            }
            return result;
        }

        extent_base_t channel_count() const { return m_data.extent()[2]; }

        channel_type::view_type channel(std::size_t channel_index) const
        {
            // return channel_type::view_type{ m_data.data() + channel_index, m_data.shape().erase(2) };
            return m_data.sub(2, static_cast<location_base_t>(channel_index));
        }

        bounds_type bounds() const
        {
            const auto b = m_data.bounds();
            return { b[0], b[1] };
        }

        view_type slice(const slice_type& s) const { return { m_data.slice({ s[0], s[1], slice_base_t{} }) }; }
    };

    struct mut_view_type
    {
        using extent_type = extent_t<2, extent_base_t>;
        using location_type = location_t<2>;
        using bounds_type = bounds_t<2>;
        using slice_type = vector_t<2, slice_base_t>;

        using data_type = storage_type::mut_view_type;
        data_type m_data;

        data_type data() const { return m_data; }
        extent_type extent() const { return pop_back(m_data.extent()); }

        operator view_type() const { return view_type{ m_data }; }

        proxy_t operator[](const location_type& loc) const { return proxy_t{ m_data[loc[0]][loc[1]] }; }

        extent_base_t channel_count() const { return m_data.extent()[2]; }

        channel_type::mut_view_type channel(std::size_t channel_index) const
        {
            // return channel_type::mut_view_type{ m_data.data() + channel_index, m_data.shape().erase(2) };
            return m_data.sub(2, static_cast<location_base_t>(channel_index));
        }

        bounds_type bounds() const
        {
            const auto b = m_data.bounds();
            return { b[0], b[1] };
        }

        mut_view_type slice(const slice_type& s) const { return { m_data.slice({ s[0], s[1], slice_base_t{} }) }; }
    };

    using extent_type = typename view_type::extent_type;
    using location_type = typename view_type::location_type;
    using bounds_type = typename view_type::bounds_type;
    using slice_type = typename view_type::slice_type;

    view_type view() const { return view_type{ m_data.view() }; }
    mut_view_type mut_view() { return mut_view_type{ m_data.mut_view() }; }

    storage_type::mut_view_type mut_data() { return m_data.mut_view(); }
    storage_type::view_type data() const { return m_data.view(); }

    image_base_t(const storage_type::extent_type& extent) : m_data{ extent } { }
    image_base_t(const extent_type& extent) : image_base_t{ append(extent, Channels) } { }
    image_base_t(const view_type& view) : m_data{ view.data() } { }

    extent_type extent() const { return view().extent(); }
    bounds_type bounds() const { return view().bounds(); }
    extent_base_t channel_count() const { return view().channel_count(); }

    operator view_type() const { return view(); }
    operator mut_view_type() { return mut_view(); }

    view_type slice(const slice_type& s) const { return view().slice(s); }
    mut_view_type slice(const slice_type& s) { return mut_view().slice(s); }

    channel_type::mut_view_type channel(std::size_t channel_index) { return mut_view().channel(channel_index); }
    channel_type::view_type channel(std::size_t channel_index) const { return view().channel(channel_index); }

    true_color_t operator[](const location_type& loc) const { return view()[loc]; }
    proxy_t operator[](const location_type& loc) { return mut_view()[loc]; }
};

using rgb_image_t = image_base_t<3>;
using rgba_image_t = image_base_t<4>;

using mask_t = array_t<float, 2>;

using segment_type = segment_t<2, location_base_t>;
using circle_type = circle_t<location_base_t>;

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
        return rgb_image_t{ rgb_image_t::extent_type{ static_cast<extent_base_t>(header.height),
                                                      static_cast<extent_base_t>(header.width) } };
    }

    static auto read_color(std::istream& is) -> true_color_t
    {
        true_color_t result = {};
        result[2] = read<byte_t>(is);
        result[1] = read<byte_t>(is);
        result[0] = read<byte_t>(is);
        return result;
    }

    static auto load_bitmap_8(std::istream& is, const dib_header& header) -> rgb_image_t
    {
        const auto padding = get_padding(header.width, header.bits_per_pixel);

        rgb_image_t result = prepare_array(header);
        auto view = result.mut_view();

        using palette_t = std::array<true_color_t, 256>;
        palette_t palette = {};

        for (std::size_t i = 0; i < 256; ++i)
        {
            palette[i] = read_color(is);
            is.ignore(1);
        }

        const auto [h, w] = view.extent();

        for (location_base_t y = h - 1; y >= 0; --y)
        {
            for (location_base_t x = 0; x < w; ++x)
            {
                view[rgb_image_t::location_type{ y, x }] = palette.at(read<byte_t>(is));
            }

            is.ignore(static_cast<std::streamsize>(padding));
        }

        return result;
    }

    static auto load_bitmap_24(std::istream& is, const dib_header& header) -> rgb_image_t
    {
        const auto padding = get_padding(header.width, header.bits_per_pixel);

        rgb_image_t result = prepare_array(header);
        auto view = result.mut_view();

        const auto [h, w] = view.extent();

        for (location_base_t y = h - 1; y >= 0; --y)
        {
            for (location_base_t x = 0; x < w; ++x)
            {
                view[rgb_image_t::location_type{ y, x }] = read_color(is);
            }

            is.ignore(static_cast<std::streamsize>(padding));
        }
        return result;
    }
};

struct save_bitmap_fn
{
    void operator()(const rgb_image_t::view_type& image, std::ostream& os) const
    {
        static const std::size_t bits_per_pixel = 24;

        const auto [h, w] = image.extent();

        const std::size_t padding = get_padding(static_cast<std::size_t>(w), bits_per_pixel);

        save_header(os, static_cast<std::size_t>(w), static_cast<std::size_t>(h), padding, bits_per_pixel, 0);

        for (location_base_t y = h - 1; y >= 0; --y)
        {
            for (location_base_t x = 0; x < w; ++x)
            {
                write_color(os, image[rgb_image_t::location_type{ y, x }]);
            }

            write_n(os, padding);
        }
    }

    static void write_color(std::ostream& os, const true_color_t& color)
    {
        write<byte_t>(os, color[2]);
        write<byte_t>(os, color[1]);
        write<byte_t>(os, color[0]);
    }

    void operator()(const rgb_image_t::view_type& image, const filepath_t& path) const
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
using binary_color_filter_t = function_ref<rgb_color_t(const rgb_color_t&, const rgb_color_t&)>;

template <std::size_t D>
void for_each(const shape_t<D>& shape, function_ref<void(const location_t<2>&)> func)
{
    const extent_base_t h = shape[0].extent;
    const extent_base_t w = shape[1].extent;

    for (location_base_t y = 0; y < h; ++y)
    {
        for (location_base_t x = 0; x < w; ++x)
        {
            func(location_t<2>{ y, x });
        }
    }
}

struct modify_fn
{
    void operator()(
        const rgb_image_t::mut_view_type& image, const rgb_image_t::location_type& loc, color_filter_t filter) const
    {
        image[loc] = filter(image[loc]);
    }

    void operator()(const rgb_image_t::mut_view_type& image, color_filter_t filter) const
    {
        for_each(image.data().shape(), [&](const rgb_image_t::location_type& loc) { (*this)(image, loc, filter); });
    }
};

struct with_fn
{
    template <class... Funcs>
    rgb_image_t operator()(const rgb_image_t::view_type& image, Funcs&&... funcs) const
    {
        rgb_image_t result = image;
        (std::invoke(std::forward<Funcs>(funcs), result.mut_view()), ...);
        return result;
    }
};

struct rotate_fn
{
    template <class T>
    auto operator()(array_view_base_t<T, 2> image, int degrees) const -> array_view_base_t<T, 2>
    {
        const auto [shape, offset] = new_shape_and_offset(image.shape(), normalize_quarter_turns(degrees));
        return { image.from_offset(offset), shape };
    }

    auto operator()(const rgb_image_t::view_type& image, int degrees) const -> rgb_image_t::view_type
    {
        const auto [shape, offset] = new_shape_and_offset(image.data().shape(), normalize_quarter_turns(degrees));
        return { { image.data().from_offset(offset), shape } };
    }

    auto operator()(const rgb_image_t::mut_view_type& image, int degrees) const -> rgb_image_t::mut_view_type
    {
        const auto [shape, offset] = new_shape_and_offset(image.data().shape(), normalize_quarter_turns(degrees));
        return { { image.data().from_offset(offset), shape } };
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

    template <class T>
    auto operator()(const array_view_base_t<T, 2>& image) const -> array_view_base_t<T, 2>
    {
        const auto [shape, offset] = new_shape_and_offset(image.shape());
        return { image.from_offset(offset), shape };
    }

    auto operator()(const rgb_image_t::view_type& image) const -> rgb_image_t::view_type
    {
        const auto [shape, offset] = new_shape_and_offset(image.data().shape());
        return { { image.data().from_offset(offset), shape } };
    }

    auto operator()(const rgb_image_t::mut_view_type& image) const -> rgb_image_t::mut_view_type
    {
        const auto [shape, offset] = new_shape_and_offset(image.data().shape());
        return { { image.data().from_offset(offset), shape } };
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

    void operator()(const rgb_image_t::location_type& loc) const
    {
        if (contains(m_image.bounds(), loc))
        {
            m_image[loc] = m_color_filter(m_image[loc]);
        }
    }
};

struct bresenham_line_fn
{
    void operator()(
        const rgb_image_t::mut_view_type& image, const segment_type& seg, const color_filter_t& color_filter) const
    {
        (*this)(seg[0], seg[1], draw_pixel_t{ image, color_filter });
    }

    void operator()(
        const rgb_image_t::location_type& start,
        const rgb_image_t::location_type& end,
        function_ref<void(const rgb_image_t::location_type&)> output) const
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
            [&](const rgb_image_t::location_type& loc)
            {
                output(loc);
                return loc != end;
            });
    }

    static void bresenham(
        rgb_image_t::location_type cur,
        const vector_t<2, location_base_t>& dir,
        const vector_t<2, location_base_t>& dist,
        int err,
        function_ref<bool(const rgb_image_t::location_type&)> output)
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
    void operator()(const rgb_image_t::mut_view_type& image, const circle_type& circle, color_filter_t color_filter) const
    {
        (*this)(circle.center, circle.radius, draw_pixel_t{ image, color_filter });
    }

    void operator()(
        const rgb_image_t::location_type& center,
        int radius,
        function_ref<void(const rgb_image_t::location_type&)> output) const
    {
        rgb_image_t::location_type cur{ radius, 0 };
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

struct paste_fn
{
    void operator()(
        const rgb_image_t::mut_view_type& dst,
        const rgb_image_t::view_type& src,
        const rgb_image_t::location_type& location,
        binary_color_filter_t filter = filters::normal) const
    {
        const auto [src_bounds, dst_bounds] = adjust_bounds(dst.data().bounds(), src.data().bounds(), append(location, 0));

        const auto clipped_dst = rgb_image_t::mut_view_type{ dst.data().slice(to_slice(dst_bounds)) };
        const auto clipped_src = rgb_image_t::view_type{ src.data().slice(to_slice(src_bounds)) };

        for_each(
            clipped_dst.data().shape(),
            [&](const rgb_image_t::location_type& loc) { clipped_dst[loc] = filter(clipped_dst[loc], clipped_src[loc]); });
    }
};

struct convolve_fn
{
    template <class Kernel>
    void operator()(
        rgb_image_t::channel_type::mut_view_type dst,
        const rgb_image_t::channel_type::view_type& src,
        const Kernel& kernel) const
    {
        const auto kernel_size = kernel.extent();

        dst = dst.slice(
            { { 0, src.shape()[0].extent - kernel_size[0] + 1 }, { 0, src.shape()[1].extent - kernel_size[1] + 1 } });

        for_each(
            dst.shape(),
            [&](const location_t<2>& loc)
            {
                const auto region = src.slice({ { loc[0], loc[0] + kernel_size[0] }, { loc[1], loc[1] + kernel_size[1] } });

                dst[loc] = true_color_t::from_float(kernel(region));
            });
    }

    template <class Kernel>
    void operator()(const rgb_image_t::mut_view_type& dst, const rgb_image_t::view_type& src, const Kernel& kernel) const
    {
        for (std::size_t z = 0; z < 3; ++z)
        {
            (*this)(dst.channel(z), src.channel(z), kernel);
        }
    }

    template <class Kernel>
    void operator()(const rgb_image_t::mut_view_type& src, const Kernel& kernel) const
    {
        rgb_image_t dst{ src.extent() };
        (*this)(dst.mut_view(), src, kernel);
        src.data().assign(dst.data());
    }
};

template <class T, class Func>
T accumulate(const mask_t::view_type& mask, const rgb_image_t::channel_type::view_type& region, T init, Func&& func)
{
    for_each(
        region.shape(),
        [&](const rgb_image_t::location_type& loc)
        {
            if (contains(mask.bounds(), loc) && contains(region.bounds(), loc))
            {
                init = func(std::move(init), mask[loc], region[loc]);
            }
        });

    return init;
}

struct dilation_kernel_t
{
    mask_t m_mask;

    rgb_image_t::channel_type::extent_type extent() const { return m_mask.extent(); }

    float operator()(const rgb_image_t::channel_type::view_type& region) const
    {
        return accumulate(
            m_mask,
            region,
            0.F,
            [](float acc, float mask_value, byte_t region_value)
            { return std::max<float>(acc, mask_value * region_value); });
    }
};

struct erosion_kernel_t
{
    mask_t m_mask;

    rgb_image_t::channel_type::extent_type extent() const { return m_mask.extent(); }

    float operator()(const rgb_image_t::channel_type::view_type& region) const
    {
        return accumulate(
                   m_mask,
                   region,
                   std::optional<float>{},
                   [](std::optional<float> acc, float mask_value, byte_t region_value)
                   {
                       if (mask_value <= 0.F)
                       {
                           return acc;
                       }

                       const float value = mask_value * static_cast<float>(region_value);
                       if (!acc || value < *acc)
                       {
                           acc = value;
                       }

                       return acc;
                   })
            .value_or(0.F);
    }
};

template <std::size_t N>
struct apply_kernel_t;

struct kernel_accumulator_t
{
    float operator()(float acc, float coeff, byte_t value) const { return acc + coeff * static_cast<float>(value); }
};

template <>
struct apply_kernel_t<1>
{
    mask_t m_mask;

    apply_kernel_t(mask_t mask) : m_mask{ std::move(mask) } { }

    rgb_image_t::channel_type::extent_type extent() const { return m_mask.extent(); }

    float operator()(const rgb_image_t::channel_type::view_type& region) const
    {
        return accumulate(m_mask, region, 0.F, kernel_accumulator_t{});
    }
};

template <>
struct apply_kernel_t<2>
{
    std::array<mask_t, 2> m_masks;

    rgb_image_t::channel_type::extent_type extent() const { return m_masks[0].extent(); }

    float operator()(const rgb_image_t::channel_type::view_type& region) const
    {
        const auto gx = accumulate(m_masks[0], region, 0.F, kernel_accumulator_t{});
        const auto gy = accumulate(m_masks[1], region, 0.F, kernel_accumulator_t{});

        return length(vector_t<2, float>{ gx, gy });
    }
};

struct percentile_kernel_t
{
    int m_rank;
    mask_t m_mask;
    mutable std::vector<float> m_values;

    percentile_kernel_t(int rank, mask_t mask) : m_rank(rank), m_mask(std::move(mask)), m_values{}
    {
        m_values.reserve(static_cast<std::size_t>(m_mask.volume()));
    }

    rgb_image_t::channel_type::extent_type extent() const { return m_mask.extent(); }

    float operator()(const rgb_image_t::channel_type::view_type& region) const
    {
        m_values.clear();

        accumulate(
            m_mask,
            region,
            std::back_inserter(m_values),
            [](auto acc, float mask_value, byte_t region_value)
            {
                *acc++ = mask_value * static_cast<float>(region_value);
                return acc;
            });

        const auto index = static_cast<std::ptrdiff_t>(m_values.size()) * m_rank / 100;
        std::nth_element(m_values.begin(), m_values.begin() + index, m_values.end());
        return m_values.begin()[index];
    }
};

}  // namespace detail

static constexpr inline auto modify = detail::modify_fn{};
static constexpr inline auto with = detail::with_fn{};

static constexpr inline auto load_bitmap = detail::load_bitmap_fn{};
static constexpr inline auto save_bitmap = detail::save_bitmap_fn{};

static constexpr inline auto rotate = detail::rotate_fn{};
static constexpr inline auto flip_horizontal = detail::flip_fn<1>{};
static constexpr inline auto flip_vertical = detail::flip_fn<0>{};

static constexpr inline auto draw_line = detail::bresenham_line_fn{};
static constexpr inline auto draw_circle = detail::bresenham_circle_fn{};
static constexpr inline auto draw_rectangle = detail::draw_rectangle_fn{};
static constexpr inline auto draw_raster = detail::draw_raster_fn{};
static constexpr inline auto paste = detail::paste_fn{};
static constexpr inline auto convolve = detail::convolve_fn{};

struct mask
{
    static mask_t rect(extent_t<2, extent_base_t> size)
    {
        mask_t mask{ size };
        detail::for_each(mask.shape(), [&](const location_t<2>& loc) { mask[loc] = 1.F; });
        return mask;
    }

    static mask_t square(extent_base_t size) { return rect(extent_t<2, extent_base_t>{ size, size }); }

    static mask_t ellipse(extent_t<2, extent_base_t> size)
    {
        mask_t mask{ size };
        const auto center = size / 2.F;
        const auto radius = vector_t<2, float>{
            (center[0] > 0.F) ? center[0] : 1.F,
            (center[1] > 0.F) ? center[1] : 1.F,
        };

        detail::for_each(
            mask.shape(),
            [&](const location_t<2>& loc)
            {
                const auto delta = loc - center;
                const float normalized_distance_squared
                    = (delta[0] * delta[0]) / (radius[0] * radius[0]) + (delta[1] * delta[1]) / (radius[1] * radius[1]);
                mask[loc] = (normalized_distance_squared <= 1.F) ? 1.F : 0.F;
            });

        return mask;
    }

    static mask_t circle(extent_base_t size) { return ellipse(extent_t<2, extent_base_t>{ size, size }); }
};

struct kernel
{
    static auto sharpen() -> detail::apply_kernel_t<1>
    {
        return create_mask<3>({ 0.F, -1.F, 0.F, -1.F, 5.F, -1.F, 0.F, -1.F, 0.F });
    }

    static auto blur() -> detail::apply_kernel_t<1>
    {
        return create_mask<3>(
            { 1.F / 16.F, 2.F / 16.F, 1.F / 16.F, 2.F / 16.F, 4.F / 16.F, 2.F / 16.F, 1.F / 16.F, 2.F / 16.F, 1.F / 16.F });
    }

    static auto emboss() -> detail::apply_kernel_t<1>
    {
        return create_mask<3>({ -2.F, -1.F, 0.F, -1.F, 1.F, 1.F, 0.F, 1.F, 2.F });
    }

    static auto edge_detect() -> detail::apply_kernel_t<1>
    {
        return create_mask<3>({ -1.F, -1.F, -1.F, -1.F, 8.F, -1.F, -1.F, -1.F, -1.F });
    }

    static auto gaussian(float sigma, location_base_t size) -> detail::apply_kernel_t<1>
    {
        mask_t mask{ { size, size } };
        const float mean = static_cast<float>(size - 1) / 2.F;
        const float sigma2 = 2.F * sigma * sigma;

        detail::for_each(
            mask.shape(),
            [&](const location_t<2>& loc)
            {
                const float dx = static_cast<float>(loc[1]) - mean;
                const float dy = static_cast<float>(loc[0]) - mean;
                const float value = std::exp(-(dx * dx + dy * dy) / sigma2);
                mask[loc] = value;
            });
        return normalize(mask);
    }

    static auto sobel() -> detail::apply_kernel_t<2>
    {
        static const auto gx_mask = create_mask<3>({ -1.F, 0.F, 1.F, -2.F, 0.F, 2.F, -1.F, 0.F, 1.F });
        static const auto gy_mask = create_mask<3>({ -1.F, -2.F, -1.F, 0.F, 0.F, 0.F, 1.F, 2.F, 1.F });
        return { gx_mask, gy_mask };
    }

    static auto cross() -> detail::apply_kernel_t<2>
    {
        static const auto gx_mask = create_mask<3>({ 0.F, 0.F, 0.F, -1.F, 0.F, 1.F, 0.F, 0.F, 0.F });
        static const auto gy_mask = create_mask<3>({ 0.F, -1.F, 0.F, 0.F, 0.F, 0.F, 0.F, 1.F, 0.F });
        return { gx_mask, gy_mask };
    }

    static auto prewitt() -> detail::apply_kernel_t<2>
    {
        static const auto gx_mask = create_mask<3>({ -1.F, 0.F, 1.F, -1.F, 0.F, 1.F, -1.F, 0.F, 1.F });
        static const auto gy_mask = create_mask<3>({ -1.F, -1.F, -1.F, 0.F, 0.F, 0.F, 1.F, 1.F, 1.F });
        return { gx_mask, gy_mask };
    }

    static auto percentile(int rank, mask_t mask) -> detail::percentile_kernel_t { return { rank, std::move(mask) }; }

    static auto median(mask_t mask) -> detail::percentile_kernel_t { return { 50, std::move(mask) }; }

    static auto dilate(mask_t mask) -> detail::dilation_kernel_t { return { std::move(mask) }; }
    static auto erode(mask_t mask) -> detail::erosion_kernel_t { return { std::move(mask) }; }

private:
    static mask_t normalize(mask_t kernel)
    {
        const float sum = std::accumulate(kernel.begin(), kernel.end(), 0.F);
        if (sum != 0.F)
        {
            std::transform(kernel.begin(), kernel.end(), kernel.begin(), [=](float value) { return value / sum; });
        }
        return kernel;
    }

    template <extent_base_t N>
    static mask_t create_mask(std::initializer_list<float> values)
    {
        if (values.size() != (N * N))
        {
            throw std::invalid_argument{ "NxN mask must have exactly N*N values" };
        }

        mask_t mask{ { N, N } };
        auto it = values.begin();
        detail::for_each(mask.shape(), [&](const location_t<2>& loc) { mask[loc] = *it++; });
        return mask;
    }
};

}  // namespace mat

}  // namespace zx

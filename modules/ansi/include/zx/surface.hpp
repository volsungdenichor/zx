#pragma once

#include <array>
#include <cuchar>
#include <zx/ansi.hpp>

namespace zx
{

namespace ansi
{

struct extent_t : public std::array<std::ptrdiff_t, 2>
{
    using base_t = std::array<std::ptrdiff_t, 2>;
    using base_t::base_t;

    extent_t(std::ptrdiff_t width, std::ptrdiff_t height) : base_t{ width, height } { }

    std::ptrdiff_t width() const { return std::get<0>(*this); }
    std::ptrdiff_t height() const { return std::get<1>(*this); }

    friend std::ostream& operator<<(std::ostream& os, const extent_t& item)
    {
        os << item.width() << "x" << item.height();
        return os;
    }
};

struct loc_t : public std::array<std::ptrdiff_t, 2>
{
    using base_t = std::array<std::ptrdiff_t, 2>;
    using base_t::base_t;

    loc_t(std::ptrdiff_t x, std::ptrdiff_t y) : base_t{ x, y } { }

    std::ptrdiff_t x() const { return std::get<0>(*this); }
    std::ptrdiff_t y() const { return std::get<1>(*this); }

    friend std::ostream& operator<<(std::ostream& os, const loc_t& loc)
    {
        os << "(" << loc.x() << ", " << loc.y() << ")";
        return os;
    }
};

template <class T, bool Const>
struct view_base_t
{
    using pointer = std::conditional_t<Const, const T*, T*>;
    using reference = std::conditional_t<Const, const T&, T&>;

    pointer m_data;
    extent_t m_extent;
    std::ptrdiff_t m_stride;

    view_base_t(pointer data, extent_t extent, std::ptrdiff_t stride)
        : m_data{ data }
        , m_extent{ extent }
        , m_stride{ stride }
    {
    }

    reference operator[](const loc_t& loc) const { return m_data[loc[1] * m_stride + loc[0]]; }

    extent_t extent() const { return m_extent; }

    view_base_t sub_view(const loc_t& origin, const extent_t& new_extent) const
    {
        loc_t adjusted_loc = {};
        extent_t adjusted_size = {};

        for (std::size_t i = 0; i < 2; ++i)
        {
            adjusted_loc[i] = std::max(origin[i], static_cast<std::ptrdiff_t>(0));
            adjusted_size[i] = adjusted_loc[i] + new_extent[i] > m_extent[i] ? m_extent[i] - adjusted_loc[i] : new_extent[i];
        }
        return view_base_t{ m_data + adjusted_loc[1] * m_stride + adjusted_loc[0], adjusted_size, m_stride };
    }

    std::pair<pointer, pointer> row(std::ptrdiff_t r) const
    {
        if (!(0 <= r && r < m_extent[1]))
        {
            return { nullptr, nullptr };
        }
        pointer row_start = m_data + r * m_stride;
        return { row_start, row_start + m_extent[0] };
    }
};

template <class T>
struct view_t : public view_base_t<T, true>
{
    using base_t = view_base_t<T, true>;
    using base_t::base_t;
};

template <class T>
struct mut_view_t : public view_base_t<T, false>
{
    using base_t = view_base_t<T, false>;
    using base_t::base_t;
};

template <class T>
struct area_t
{
    extent_t m_extent;
    std::vector<T> m_data;

    using view_type = view_t<T>;
    using mut_view_type = mut_view_t<T>;

    using mut_reference = typename mut_view_type::reference;
    using reference = typename view_type::reference;

    area_t(extent_t extent, T fill = T{})
        : m_extent{ extent }
        , m_data(static_cast<std::size_t>(extent.width() * extent.height()), fill)
    {
    }

    extent_t extent() const { return m_extent; }

    view_type view() const { return view_type{ m_data.data(), m_extent, m_extent.width() }; }
    mut_view_type mut_view() { return mut_view_type{ m_data.data(), m_extent, m_extent.width() }; }

    operator view_type() const { return view(); }

    reference operator[](const loc_t& loc) const { return view()[loc]; }
    mut_reference operator[](const loc_t& loc) { return mut_view()[loc]; }
};

struct glyph_t
{
    char32_t m_data;

    glyph_t() = default;

    glyph_t(char32_t data) : m_data(data) { }

    glyph_t(std::string_view txt) : glyph_t()
    {
        std::setlocale(LC_ALL, "en_US.utf8");
        std::mbstate_t state{};
        std::size_t rc = std::mbrtoc32(&m_data, txt.data(), txt.size(), &state);
        assert(rc != std::size_t(0) && rc != std::size_t(-1) && rc != std::size_t(-2));
    }

    glyph_t(char ch) : glyph_t(std::string_view(&ch, 1)) { }

    friend bool operator==(const glyph_t& lhs, const glyph_t& rhs) { return lhs.m_data == rhs.m_data; }
    friend bool operator!=(const glyph_t& lhs, const glyph_t& rhs) { return !(lhs == rhs); }
    friend bool operator<(const glyph_t& lhs, const glyph_t& rhs) { return lhs.m_data < rhs.m_data; }
    friend bool operator>(const glyph_t& lhs, const glyph_t& rhs) { return rhs < lhs; }
    friend bool operator<=(const glyph_t& lhs, const glyph_t& rhs) { return !(lhs > rhs); }
    friend bool operator>=(const glyph_t& lhs, const glyph_t& rhs) { return !(lhs < rhs); }

    friend std::ostream& operator<<(std::ostream& os, const glyph_t& item)
    {
        std::setlocale(LC_ALL, "en_US.utf8");
        std::mbstate_t state{};
        char out[MB_LEN_MAX]{};
        std::size_t rc = std::c32rtomb(out, item.m_data, &state);
        assert(rc != static_cast<std::size_t>(-1));
        std::copy(out, out + rc, std::ostreambuf_iterator<char>(os));
        return os;
    }
};

struct cell_t
{
    glyph_t glyph;
    style_info_t style;

    cell_t() = default;

    cell_t(glyph_t g, style_info_t s = {}) : glyph(g), style(s) { }

    friend std::ostream& operator<<(std::ostream& os, const cell_t& item)
    {
        os << "(cell_t glyph:" << item.glyph << " style:" << item.style << ")";
        return os;
    }
};

inline std::string render_line(std::pair<const cell_t*, const cell_t*> range)
{
    std::string result = {};
    auto current_style = style_info_t{};
    bool reset_needed = false;
    for (const cell_t* it = range.first; it != range.second; ++it)
    {
        if (it->style != current_style)
        {
            result += str(make_ansi_code(it->style));
            current_style = it->style;
            reset_needed = true;
        }
        result += str(it->glyph);
    }
    if (reset_needed)
    {
        result += str(escape_sequence_t{ 0 });
    }
    return result;
}

using surface_t = area_t<cell_t>;

inline std::vector<std::string> render_lines(const surface_t::view_type& surface)
{
    std::vector<std::string> lines = {};
    lines.reserve(static_cast<std::size_t>(surface.extent()[1]));
    for (std::ptrdiff_t y = 0; y < surface.extent()[1]; ++y)
    {
        lines.push_back(render_line(surface.row(y)));
    }
    return lines;
}

inline std::string render(const surface_t::view_type& surface)
{
    auto lines = render_lines(surface);
    std::string result = {};
    for (std::size_t i = 0; i < lines.size(); ++i)
    {
        if (i != 0)
        {
            result += "\n";
        }
        result += lines[i];
    }
    return result;
}

}  // namespace ansi

}  // namespace zx

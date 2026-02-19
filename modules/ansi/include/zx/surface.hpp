#pragma once

#include <array>
#include <zx/ansi.hpp>

namespace zx
{

namespace ansi
{

inline char32_t utf8_to_char32(std::string_view s)
{
    if (s.empty())
        return U'\0';

    unsigned char byte = static_cast<unsigned char>(s[0]);

    if ((byte & 0x80) == 0)
    {
        return byte;
    }

    auto [codepoint, remaining] = std::invoke(
        [&]() -> std::tuple<char32_t, std::size_t>
        {
            if ((byte & 0xE0) == 0xC0)
            {
                return { byte & 0x1F, 1 };
            }
            else if ((byte & 0xF0) == 0xE0)
            {
                return { byte & 0x0F, 2 };
            }
            else if ((byte & 0xF8) == 0xF0)
            {
                return { byte & 0x07, 3 };
            }
            return { 0, 0 };
        });

    for (std::size_t i = 0; i < remaining && i + 1 < s.length(); ++i)
    {
        codepoint = (codepoint << 6) | (static_cast<unsigned char>(s[i + 1]) & 0x3F);
    }

    return codepoint;
}

inline std::string& char32_to_utf8(std::string& result, char32_t codepoint)
{
    if (codepoint <= 0x7F)
    {
        result += static_cast<char>(codepoint);
    }
    else if (codepoint <= 0x7FF)
    {
        result += static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F));
        result += static_cast<char>(0x80 | ((codepoint >> 0) & 0x3F));
    }
    else if (codepoint <= 0xFFFF)
    {
        result += static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F));
        result += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        result += static_cast<char>(0x80 | ((codepoint >> 0) & 0x3F));
    }
    else if (codepoint <= 0x10FFFF)
    {
        result += static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07));
        result += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
        result += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        result += static_cast<char>(0x80 | ((codepoint >> 0) & 0x3F));
    }

    return result;
}

inline std::string char32_to_utf8(char32_t codepoint)
{
    std::string result;
    char32_to_utf8(result, codepoint);
    return result;
}

struct cell_t
{
    char32_t ch;
    style_info_t style;

    cell_t() : ch{ U' ' }, style{} { }

    cell_t(char32_t ch, style_info_t style = {}) : ch{ ch }, style{ style } { }
    cell_t(std::string_view s, style_info_t style = {}) : ch{ utf8_to_char32(std::string(s)) }, style{ style } { }

    friend std::ostream& operator<<(std::ostream& os, const cell_t& item)
    {
        os << "(cell_t ch:" << item.ch << " style:" << item.style << ")";
        return os;
    }
};

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

template <bool Const>
struct surface_view_base_t
{
    using pointer = std::conditional_t<Const, const cell_t*, cell_t*>;
    using reference = std::conditional_t<Const, const cell_t&, cell_t&>;

    pointer m_data;
    extent_t m_extent;
    std::ptrdiff_t m_stride;

    surface_view_base_t(pointer data, extent_t extent, std::ptrdiff_t stride)
        : m_data{ data }
        , m_extent{ extent }
        , m_stride{ stride }
    {
    }

    reference operator[](const loc_t& loc) const { return m_data[loc[1] * m_stride + loc[0]]; }

    extent_t extent() const { return m_extent; }

    surface_view_base_t sub_view(const loc_t& origin, const extent_t& new_extent) const
    {
        loc_t adjusted_loc = {};
        extent_t adjusted_size = {};

        for (std::size_t i = 0; i < 2; ++i)
        {
            adjusted_loc[i] = std::max(origin[i], static_cast<std::ptrdiff_t>(0));
        }

        for (std::size_t i = 0; i < 2; ++i)
        {
            adjusted_size[i] = adjusted_loc[i] + new_extent[i] > m_extent[i] ? m_extent[i] - adjusted_loc[i] : new_extent[i];
        }
        return surface_view_base_t{ m_data + adjusted_loc[1] * m_stride + adjusted_loc[0], adjusted_size, m_stride };
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

struct surface_view_t : public surface_view_base_t<true>
{
    using base_t = surface_view_base_t<true>;
    using base_t::base_t;
};

struct surface_mut_view_t : public surface_view_base_t<false>
{
    using base_t = surface_view_base_t<false>;
    using base_t::base_t;
};

struct surface_t
{
    extent_t m_extent;
    std::vector<cell_t> m_cells;

    using view_type = surface_view_t;
    using mut_view_type = surface_mut_view_t;

    using mut_reference = mut_view_type::reference;
    using reference = view_type::reference;

    surface_t(extent_t extent, cell_t fill = {})
        : m_extent{ extent }
        , m_cells(static_cast<std::size_t>(extent.width() * extent.height()), fill)
    {
    }

    extent_t extent() const { return m_extent; }

    view_type view() const { return view_type{ m_cells.data(), m_extent, m_extent.width() }; }
    mut_view_type mut_view() { return mut_view_type{ m_cells.data(), m_extent, m_extent.width() }; }

    operator view_type() const { return view(); }

    reference operator[](const loc_t& loc) const { return view()[loc]; }
    mut_reference operator[](const loc_t& loc) { return mut_view()[loc]; }
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
        char32_to_utf8(result, it->ch);
    }
    if (reset_needed)
    {
        result += str(escape_sequence_t{ 0 });
    }
    return result;
}

inline std::vector<std::string> render_lines(const surface_view_t& surface)
{
    std::vector<std::string> lines = {};
    lines.reserve(static_cast<std::size_t>(surface.extent()[1]));
    for (std::ptrdiff_t y = 0; y < surface.extent()[1]; ++y)
    {
        lines.push_back(render_line(surface.row(y)));
    }
    return lines;
}

inline std::string render(const surface_view_t& surface)
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

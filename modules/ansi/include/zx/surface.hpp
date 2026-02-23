#pragma once

#include <array>
#include <string_view>
#include <zx/ansi.hpp>
#include <zx/array.hpp>
#include <zx/string.hpp>

namespace zx
{

namespace ansi
{

struct cell_t
{
    glyph_t glyph = glyph_t(' ');
    style_info_t style = {};

    cell_t(glyph_t g = {}, style_info_t s = {}) : glyph(g), style(s) { }

    friend std::ostream& operator<<(std::ostream& os, const cell_t& item)
    {
        os << "(cell_t glyph:" << item.glyph << " style:" << item.style << ")";
        return os;
    }
};

inline std::string render_line(const arrays::array_t<cell_t, 1>::view_type& range)
{
    std::string result = {};
    auto current_style = style_info_t{};
    bool reset_needed = false;
    for (const cell_t& cell : range)
    {
        if (cell.style != current_style)
        {
            result += str(make_ansi_code(cell.style));
            current_style = cell.style;
            reset_needed = true;
        }
        result += str(cell.glyph);
    }
    if (reset_needed)
    {
        result += str(escape_sequence_t{ 0 });
    }
    return result;
}

using surface_t = arrays::array_t<cell_t, 2>;

inline std::vector<std::string> render_lines(const surface_t::view_type& surface)
{
    constexpr auto row = [](const surface_t::view_type& s, std::ptrdiff_t y) -> arrays::array_t<cell_t, 1>::view_type
    {
        const auto slice = s.slice({ arrays::slice_base_t{ y, y + 1, {} }, arrays::slice_base_t{} });
        return arrays::array_t<cell_t, 1>::view_type{
            slice.m_data, arrays::shape_t<1>{ { slice.size()[1], slice.stride()[1], slice.start()[0] } }
        };
    };
    std::cerr << surface.shape().stride() << std::endl;
    std::vector<std::string> lines = {};
    lines.reserve(static_cast<std::size_t>(surface.size()[0]));
    for (std::ptrdiff_t y = 0; y < surface.size()[0]; ++y)
    {
        lines.push_back(render_line(row(surface, y)));
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

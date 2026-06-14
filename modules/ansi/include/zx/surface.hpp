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
    using value_type = string_t::value_type;

    value_type code_point = value_type(' ');
    style_info_t style = {};

    cell_t(value_type cp = {}, style_info_t s = {}) : code_point(cp), style(s) { }

    friend std::ostream& operator<<(std::ostream& os, const cell_t& item)
    {
        os << "(cell_t code_point:" << item.code_point << " style:" << item.style << ")";
        return os;
    }
};

using surface_t = arrays::array_t<cell_t, 2>;

inline std::string render(const surface_t::view_type& surface)
{
    std::string out = str(cursor_move_t{});

    style_info_t current_style = {};
    out += str(escape_sequence_t{ 0 }, make_ansi_code(current_style));

    for (std::ptrdiff_t y = 0; y < surface.size()[0]; ++y)
    {
        for (std::ptrdiff_t x = 0; x < surface.size()[1]; ++x)
        {
            const cell_t& cell = surface[{ y, x }];
            if (cell.style != current_style)
            {
                out += str(escape_sequence_t{ 0 }, make_ansi_code(cell.style));
                current_style = cell.style;
            }
            out += str(cell.code_point);
        }

        if (y + 1 < surface.size()[0])
        {
            out += "\r\n";
        }
    }

    out += str(escape_sequence_t{ 0 });
    return out;
}

inline std::string render_diff(const surface_t::view_type& prev, const surface_t::view_type& next)
{
    std::string out = {};
    style_info_t current_style = {};
    bool style_emitted = false;
    surface_t::location_type last = { -1, -1 };

    const bool same_size = prev.size() == next.size();

    for (std::ptrdiff_t y = 0; y < next.size()[0]; ++y)
    {
        for (std::ptrdiff_t x = 0; x < next.size()[1]; ++x)
        {
            const auto pos = surface_t::location_type{ y, x };
            const cell_t& cell = next[pos];

            if (same_size)
            {
                const cell_t& prev_cell = prev[pos];
                if (prev_cell.code_point == cell.code_point && prev_cell.style == cell.style)
                {
                    continue;
                }
            }

            if (last[0] != y || last[1] != x)
            {
                out += str(cursor_move_t{ static_cast<int>(y) + 1, static_cast<int>(x) + 1 });
                last = { y, x };
            }

            if (!style_emitted || cell.style != current_style)
            {
                out += str(escape_sequence_t{ 0 }, make_ansi_code(cell.style));
                current_style = cell.style;
                style_emitted = true;
            }

            out += str(cell.code_point);
            ++last[1];
        }
    }

    if (style_emitted)
    {
        out += str(escape_sequence_t{ 0 });
    }

    return out;
}

struct box_style_t
{
    string_t::value_type horizontal = string_t::value_type('-');
    string_t::value_type vertical = string_t::value_type('|');
    string_t::value_type top_left = string_t::value_type('+');
    string_t::value_type top_right = string_t::value_type('+');
    string_t::value_type bottom_left = string_t::value_type('+');
    string_t::value_type bottom_right = string_t::value_type('+');
};

const inline box_style_t default_box_style
    = { string_t::value_type{ "-" }, string_t::value_type{ "|" }, string_t::value_type{ "+" },
        string_t::value_type{ "+" }, string_t::value_type{ "+" }, string_t::value_type{ "+" } };
const inline box_style_t double_box_style
    = { string_t::value_type{ "=" }, string_t::value_type{ "‖" }, string_t::value_type{ "╔" },
        string_t::value_type{ "╗" }, string_t::value_type{ "╚" }, string_t::value_type{ "╝" } };
const inline box_style_t rounded_box_style
    = { string_t::value_type{ "─" }, string_t::value_type{ "│" }, string_t::value_type{ "╭" },
        string_t::value_type{ "╮" }, string_t::value_type{ "╰" }, string_t::value_type{ "╯" } };
const inline box_style_t heavy_box_style
    = { string_t::value_type{ "━" }, string_t::value_type{ "┃" }, string_t::value_type{ "┏" },
        string_t::value_type{ "┓" }, string_t::value_type{ "┗" }, string_t::value_type{ "┛" } };

void draw_border(
    surface_t::mut_view_type surface,
    const surface_t::bounds_type& bounds,
    const box_style_t& box_style = {},
    const style_info_t& style = {})
{
    const auto top_left = zx::mat::lower(bounds);
    const auto bottom_right = zx::mat::upper(bounds);

    for (std::ptrdiff_t x = top_left[1] + 1; x < bottom_right[1]; ++x)
    {
        surface[{ top_left[0], x }] = cell_t{ box_style.horizontal, style };
        surface[{ bottom_right[0], x }] = cell_t{ box_style.horizontal, style };
    }

    for (std::ptrdiff_t y = top_left[0] + 1; y < bottom_right[0]; ++y)
    {
        surface[{ y, top_left[1] }] = cell_t{ box_style.vertical, style };
        surface[{ y, bottom_right[1] }] = cell_t{ box_style.vertical, style };
    }

    surface[{ top_left[0], top_left[1] }] = cell_t{ box_style.top_left, style };
    surface[{ top_left[0], bottom_right[1] }] = cell_t{ box_style.top_right, style };
    surface[{ bottom_right[0], top_left[1] }] = cell_t{ box_style.bottom_left, style };
    surface[{ bottom_right[0], bottom_right[1] }] = cell_t{ box_style.bottom_right, style };
}

}  // namespace ansi

}  // namespace zx

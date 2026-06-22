#pragma once

#include <array>
#include <functional>
#include <string_view>
#include <zx/ansi.hpp>
#include <zx/array.hpp>
#include <zx/string.hpp>

namespace zx
{

namespace ansi
{

using symbol_t = code_point_t;

struct cell_t
{
    symbol_t symbol = symbol_t(' ');
    style_t style = {};

    cell_t(symbol_t symbol_ = symbol_t(' '), style_t style_ = {}) : symbol(symbol_), style(style_) { }

    friend bool operator==(const cell_t& lhs, const cell_t& rhs)
    {
        return std::tie(lhs.symbol, lhs.style) == std::tie(rhs.symbol, rhs.style);
    }

    friend bool operator!=(const cell_t& lhs, const cell_t& rhs) { return !(lhs == rhs); }

    friend std::ostream& operator<<(std::ostream& os, const cell_t& item)
    {
        os << "(cell_t symbol:" << item.symbol << " style:" << item.style << ")";
        return os;
    }
};

using surface_t = arrays::array_t<cell_t, 2>;
using surface_view_t = surface_t::view_type;
using surface_mut_view_t = surface_t::mut_view_type;

using symbols_view_t = arrays::array_view_t<symbol_t, 2>;
using symbols_mut_view_t = arrays::array_mut_view_t<symbol_t, 2>;

using styles_view_t = arrays::array_view_t<style_t, 2>;
using styles_mut_view_t = arrays::array_mut_view_t<style_t, 2>;

namespace detail
{

template <class CellType>
struct cell_layout_validator
{
    static_assert(std::is_standard_layout_v<CellType>, "cell_t must be standard-layout for offsetof");

    static constexpr std::ptrdiff_t symbol_offset = offsetof(CellType, symbol);
    static constexpr std::ptrdiff_t style_offset = offsetof(CellType, style);

    static_assert(symbol_offset % alignof(symbol_t) == 0, "symbol offset must satisfy alignment requirements");
    static_assert(style_offset % alignof(style_t) == 0, "style offset must satisfy alignment requirements");
};

template <class U, class T, std::size_t D>
arrays::array_view_base_t<U, D> shift(arrays::array_view_base_t<T, D> view, std::ptrdiff_t member_offset)
{
    U* p = const_cast<U*>(reinterpret_cast<const U*>(reinterpret_cast<const std::byte*>(view.data()) + member_offset));
    return { p, view.shape() };
}

inline constexpr auto cell_layout = cell_layout_validator<cell_t>{};

}  // namespace detail

inline symbols_mut_view_t mut_symbols(surface_mut_view_t surface)
{
    return detail::shift<symbol_t>(surface, detail::cell_layout.symbol_offset);
}

inline symbols_view_t symbols(surface_view_t surface)
{
    return detail::shift<const symbol_t>(surface, detail::cell_layout.symbol_offset);
}

inline styles_mut_view_t mut_styles(surface_mut_view_t surface)
{
    return detail::shift<style_t>(surface, detail::cell_layout.style_offset);
}

inline styles_view_t styles(surface_view_t surface)
{
    return detail::shift<const style_t>(surface, detail::cell_layout.style_offset);
}

inline std::string clear_screen()
{
    return "\033[2J\033[H";
}

inline std::string render(surface_view_t surface)
{
    std::string out = str(cursor_move_t{});

    style_t current_style = {};
    out += str(escape_sequence_t{ 0 }, make_ansi_code(current_style));

    for (arrays::location_base_t y = 0; y < surface.size()[0]; ++y)
    {
        for (arrays::location_base_t x = 0; x < surface.size()[1]; ++x)
        {
            const cell_t& cell = surface[{ y, x }];
            if (cell.style != current_style)
            {
                out += str(escape_sequence_t{ 0 }, make_ansi_code(cell.style));
                current_style = cell.style;
            }
            out += str(cell.symbol);
        }

        if (y + 1 < surface.size()[0])
        {
            out += "\r\n";
        }
    }

    out += str(escape_sequence_t{ 0 });
    return out;
}

inline cursor_move_t cursor_move(const surface_t::location_type& pos)
{
    return cursor_move_t{ static_cast<int>(pos[0]) + 1, static_cast<int>(pos[1]) + 1 };
}

inline std::string render_diff(surface_view_t prev, surface_view_t next)
{
    const bool same_size = prev.size() == next.size();

    std::string out = {};
    style_t current_style = {};
    bool style_emitted = false;
    surface_t::location_type last = { -1, -1 };

    for (arrays::location_base_t y = 0; y < next.size()[0]; ++y)
    {
        for (arrays::location_base_t x = 0; x < next.size()[1]; ++x)
        {
            const auto pos = surface_t::location_type{ y, x };
            const cell_t& cell = next[pos];

            if (same_size)
            {
                const cell_t& prev_cell = prev[pos];
                if (prev_cell == cell)
                {
                    continue;
                }
            }

            if (pos != last)
            {
                out += str(cursor_move(pos));
                last = pos;
            }

            if (!style_emitted || cell.style != current_style)
            {
                out += str(escape_sequence_t{ 0 }, make_ansi_code(cell.style));
                current_style = cell.style;
                style_emitted = true;
            }

            out += str(cell.symbol);
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
    code_point_t horizontal = code_point_t('-');
    code_point_t vertical = code_point_t('|');
    code_point_t top_left = code_point_t('+');
    code_point_t top_right = code_point_t('+');
    code_point_t bottom_left = code_point_t('+');
    code_point_t bottom_right = code_point_t('+');
};

const inline box_style_t default_box_style = { code_point_t{ "-" }, code_point_t{ "|" }, code_point_t{ "+" },
                                               code_point_t{ "+" }, code_point_t{ "+" }, code_point_t{ "+" } };
const inline box_style_t double_box_style = { code_point_t{ "=" }, code_point_t{ "‖" }, code_point_t{ "╔" },
                                              code_point_t{ "╗" }, code_point_t{ "╚" }, code_point_t{ "╝" } };
const inline box_style_t rounded_box_style = { code_point_t{ "─" }, code_point_t{ "│" }, code_point_t{ "╭" },
                                               code_point_t{ "╮" }, code_point_t{ "╰" }, code_point_t{ "╯" } };
const inline box_style_t heavy_box_style = { code_point_t{ "━" }, code_point_t{ "┃" }, code_point_t{ "┏" },
                                             code_point_t{ "┓" }, code_point_t{ "┗" }, code_point_t{ "┛" } };

inline void draw_border(
    surface_mut_view_t surface,
    const surface_t::bounds_type& bounds,
    const box_style_t& box_style = {},
    const style_t& style = {})
{
    const auto top_left = bounds.get(mat::side_t::first);
    const auto bottom_right = bounds.get(mat::side_t::last);

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

    surface[bounds.get({ mat::side_t::first, mat::side_t::first })] = cell_t{ box_style.top_left, style };
    surface[bounds.get({ mat::side_t::first, mat::side_t::last })] = cell_t{ box_style.top_right, style };
    surface[bounds.get({ mat::side_t::last, mat::side_t::first })] = cell_t{ box_style.bottom_left, style };
    surface[bounds.get({ mat::side_t::last, mat::side_t::last })] = cell_t{ box_style.bottom_right, style };
}

inline void draw_text(
    const surface_t::bounds_type& dest,
    const string_t& text,
    const std::function<void(symbol_t, const surface_t::location_type&)>& draw_cell)
{
    auto pos = mat::lower(dest);
    for (const auto& ch : text)
    {
        draw_cell(ch, pos);
        if (++pos[1] >= mat::upper(dest[1]))
        {
            pos[1] = mat::lower(dest[1]);
            if (++pos[0] >= mat::upper(dest[0]))
            {
                break;
            }
        }
    }
}

inline void draw_text(
    surface_mut_view_t surface, const surface_t::bounds_type& dest, const string_t& text, const style_t& style = {})
{
    draw_text(dest, text, [&](symbol_t symbol, const surface_t::location_type& pos) { surface[pos] = { symbol, style }; });
}

}  // namespace ansi

}  // namespace zx

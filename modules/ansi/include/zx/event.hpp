#pragma once

#include <cstdint>
#include <utility>
#include <variant>
#include <zx/mat.hpp>
#include <zx/string.hpp>

namespace zx
{
namespace ansi
{

enum class key_modifiers_t : std::uint8_t
{
    none = 0,
    shift = 1 << 0,
    ctrl = 1 << 1,
    alt = 1 << 2,
};

constexpr inline key_modifiers_t operator|(key_modifiers_t a, key_modifiers_t b)
{
    return static_cast<key_modifiers_t>(static_cast<std::uint8_t>(a) | static_cast<std::uint8_t>(b));
}

constexpr inline key_modifiers_t operator&(key_modifiers_t a, key_modifiers_t b)
{
    return static_cast<key_modifiers_t>(static_cast<std::uint8_t>(a) & static_cast<std::uint8_t>(b));
}

constexpr inline key_modifiers_t& operator|=(key_modifiers_t& a, key_modifiers_t b)
{
    a = a | b;
    return a;
}

constexpr inline bool has(key_modifiers_t value, key_modifiers_t flag)
{
    return (static_cast<std::uint8_t>(value & flag) != 0);
}

inline std::ostream& operator<<(std::ostream& os, const key_modifiers_t& item)
{
    if (item == key_modifiers_t::none)
    {
        os << "none";
        return os;
    }
    bool first = true;
    if (has(item, key_modifiers_t::shift))
    {
        os << "shift";
        first = false;
    }
    if (has(item, key_modifiers_t::ctrl))
    {
        if (!first)
        {
            os << "|";
        }
        os << "ctrl";
        first = false;
    }
    if (has(item, key_modifiers_t::alt))
    {
        if (!first)
        {
            os << "|";
        }
        os << "alt";
    }
    return os;
}

enum class key_t : std::uint16_t
{
    none = 0,
    up,
    down,
    left,
    right,
    enter,
    escape,
    backspace,
    del,
    tab,
    backtab,
    home,
    end,
    page_up,
    page_down,
    f1,
    f2,
    f3,
    f4,
    f5,
    f6,
    f7,
    f8,
    f9,
    f10,
    f11,
    f12,
};

inline std::ostream& operator<<(std::ostream& os, const key_t& item)
{
    switch (item)
    {
        case key_t::none: os << "none"; break;
        case key_t::up: os << "up"; break;
        case key_t::down: os << "down"; break;
        case key_t::left: os << "left"; break;
        case key_t::right: os << "right"; break;
        case key_t::enter: os << "enter"; break;
        case key_t::escape: os << "escape"; break;
        case key_t::backspace: os << "backspace"; break;
        case key_t::del: os << "del"; break;
        case key_t::tab: os << "tab"; break;
        case key_t::backtab: os << "backtab"; break;
        case key_t::home: os << "home"; break;
        case key_t::end: os << "end"; break;
        case key_t::page_up: os << "page_up"; break;
        case key_t::page_down: os << "page_down"; break;
        case key_t::f1: os << "f1"; break;
        case key_t::f2: os << "f2"; break;
        case key_t::f3: os << "f3"; break;
        case key_t::f4: os << "f4"; break;
        case key_t::f5: os << "f5"; break;
        case key_t::f6: os << "f6"; break;
        case key_t::f7: os << "f7"; break;
        case key_t::f8: os << "f8"; break;
        case key_t::f9: os << "f9"; break;
        case key_t::f10: os << "f10"; break;
        case key_t::f11: os << "f11"; break;
        case key_t::f12: os << "f12"; break;
        default: os << "unknown"; break;
    }
    return os;
}

struct key_event_t
{
    using value_t = std::variant<code_point_t, key_t>;

    value_t value = code_point_t{};
    key_modifiers_t modifiers = key_modifiers_t::none;

    const key_t* special() const { return std::get_if<key_t>(&value); }

    const code_point_t* code_point() const { return std::get_if<code_point_t>(&value); }

    bool is_printable() const
    {
        if (!std::holds_alternative<code_point_t>(value))
        {
            return false;
        }
        return std::get<code_point_t>(value).m_data >= 0x20;
    }

    friend std::ostream& operator<<(std::ostream& os, const key_event_t& item)
    {
        os << "key_event_t{";
        if (const auto cp = item.code_point())
        {
            os << *cp;
        }
        else if (const auto s = item.special())
        {
            os << *s;
        }
        os << " modifiers=" << item.modifiers << "}";
        return os;
    }
};

struct resize_event_t
{
    mat::vector_t<2, std::ptrdiff_t> new_size;

    friend std::ostream& operator<<(std::ostream& os, const resize_event_t& item)
    {
        os << "resize_event_t{" << item.new_size << "}";
        return os;
    }
};

enum class mouse_button_t : std::uint8_t
{
    none = 0,
    left,
    middle,
    right,
};

inline std::ostream& operator<<(std::ostream& os, const mouse_button_t& item)
{
    switch (item)
    {
        case mouse_button_t::none: os << "none"; break;
        case mouse_button_t::left: os << "left"; break;
        case mouse_button_t::middle: os << "middle"; break;
        case mouse_button_t::right: os << "right"; break;
        default: os << "unknown"; break;
    }
    return os;
}

enum class mouse_event_kind_t : std::uint8_t
{
    move = 0,
    down,
    up,
    drag,
    scroll,
};

inline std::ostream& operator<<(std::ostream& os, const mouse_event_kind_t& item)
{
    switch (item)
    {
        case mouse_event_kind_t::move: os << "move"; break;
        case mouse_event_kind_t::down: os << "down"; break;
        case mouse_event_kind_t::up: os << "up"; break;
        case mouse_event_kind_t::drag: os << "drag"; break;
        case mouse_event_kind_t::scroll: os << "scroll"; break;
        default: os << "unknown"; break;
    }
    return os;
}

struct mouse_event_t
{
    mouse_event_kind_t kind = mouse_event_kind_t::move;
    mouse_button_t button = mouse_button_t::none;
    mat::vector_t<2, std::ptrdiff_t> location;
    std::ptrdiff_t scroll_y = 0;
    key_modifiers_t modifiers = key_modifiers_t::none;

    friend std::ostream& operator<<(std::ostream& os, const mouse_event_t& item)
    {
        os << "mouse_event_t{" << item.kind << " " << item.button << " " << item.location << " " << item.scroll_y << " "
           << "modifiers=" << item.modifiers << "}";
        return os;
    }
};

struct tick_event_t
{
    friend std::ostream& operator<<(std::ostream& os, const tick_event_t&)
    {
        os << "tick_event_t{}";
        return os;
    }
};

struct quit_event_t
{
    friend std::ostream& operator<<(std::ostream& os, const quit_event_t&)
    {
        os << "quit_event_t{}";
        return os;
    }
};

using event_t = std::variant<key_event_t, resize_event_t, mouse_event_t, tick_event_t, quit_event_t>;

}  // namespace ansi
}  // namespace zx

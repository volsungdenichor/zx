#pragma once

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>
#include <numeric>
#include <optional>
#include <sstream>
#include <vector>

namespace zx
{

namespace ansi
{

struct str_fn
{
    template <class... Args>
    std::string operator()(Args&&... args) const
    {
        std::ostringstream os;
        (os << ... << args);
        return os.str();
    }
} str{};

struct escape_sequence_t : public std::vector<int>
{
    using base_t = std::vector<int>;
    using base_t::base_t;

    friend std::ostream& operator<<(std::ostream& os, const escape_sequence_t& item)
    {
        os << "\033[";
        for (size_t i = 0; i < item.size(); ++i)
        {
            if (i != 0)
            {
                os << ";";
            }
            os << item[i];
        }
        os << "m";
        return os;
    }

    friend escape_sequence_t& operator+=(escape_sequence_t& lhs, const escape_sequence_t& rhs)
    {
        lhs.insert(lhs.end(), rhs.begin(), rhs.end());
        return lhs;
    }

    friend escape_sequence_t operator+(escape_sequence_t lhs, const escape_sequence_t& rhs)
    {
        lhs += rhs;
        return lhs;
    }
};

struct color_t
{
    using value_type = std::uint8_t;
    value_type m_value;

    explicit constexpr color_t(value_type value) : m_value(value) { }

    constexpr value_type value() const { return m_value; }

    static constexpr color_t from_rgb(value_type r, value_type g, value_type b)
    {
        const int v[] = { (r * 6) / 256, (g * 6) / 256, (b * 6) / 256 };
        return color_t{ static_cast<value_type>(16 + 36 * v[0] + 6 * v[1] + v[2]) };
    }

    static constexpr color_t from_grayscale(value_type level)
    {
        return color_t{ static_cast<value_type>(232 + (level * 24) / 256) };
    }

    static const color_t black;
    static const color_t red;
    static const color_t green;
    static const color_t yellow;
    static const color_t blue;
    static const color_t magenta;
    static const color_t cyan;
    static const color_t white;
    static const color_t bright_black;
    static const color_t bright_red;
    static const color_t bright_green;
    static const color_t bright_yellow;
    static const color_t bright_blue;
    static const color_t bright_magenta;
    static const color_t bright_cyan;
    static const color_t bright_white;

    static std::optional<color_t> parse(const std::string& text)
    {
        static const std::vector<std::pair<std::string_view, color_t>> color_map
            = { { "black", color_t::black },
                { "red", color_t::red },
                { "green", color_t::green },
                { "yellow", color_t::yellow },
                { "blue", color_t::blue },
                { "magenta", color_t::magenta },
                { "cyan", color_t::cyan },
                { "white", color_t::white },
                { "bright_black", color_t::bright_black },
                { "bright_red", color_t::bright_red },
                { "bright_green", color_t::bright_green },
                { "bright_yellow", color_t::bright_yellow },
                { "bright_blue", color_t::bright_blue },
                { "bright_magenta", color_t::bright_magenta },
                { "bright_cyan", color_t::bright_cyan },
                { "bright_white", color_t::bright_white } };

        if (const auto iter
            = std::find_if(color_map.begin(), color_map.end(), [&text](const auto& pair) { return pair.first == text; });
            iter != color_map.end())
        {
            return color_t{ iter->second };
        }
        if (text.find("0x") == 0 && text.length() == 8)
        {
            try
            {
                unsigned long hex_value = std::stoul(text.substr(2), nullptr, 16);
                const std::uint8_t r = (hex_value >> 16) & 0xFF;
                const std::uint8_t g = (hex_value >> 8) & 0xFF;
                const std::uint8_t b = (hex_value >> 0) & 0xFF;

                return from_rgb(r, g, b);
            }
            catch (...)
            {
                return std::nullopt;
            }
        }
        if (text.find("gray:") == 0 || text.find("grey:") == 0)
        {
            try
            {
                size_t colon_pos = text.find(':');
                int level = std::stoi(text.substr(colon_pos + 1));
                if (level >= 0 && level <= 255)
                {
                    return from_grayscale(static_cast<value_type>(level));
                }
            }
            catch (...)
            {
                return std::nullopt;
            }
        }

        return std::nullopt;
    }

    friend constexpr bool operator==(const color_t& lhs, const color_t& rhs) { return lhs.m_value == rhs.m_value; }
    friend constexpr bool operator!=(const color_t& lhs, const color_t& rhs) { return !(lhs == rhs); }
    friend constexpr bool operator<(const color_t& lhs, const color_t& rhs) { return lhs.m_value < rhs.m_value; }
    friend constexpr bool operator>(const color_t& lhs, const color_t& rhs) { return rhs < lhs; }
    friend constexpr bool operator<=(const color_t& lhs, const color_t& rhs) { return !(lhs > rhs); }
    friend constexpr bool operator>=(const color_t& lhs, const color_t& rhs) { return !(lhs < rhs); }

    friend std::ostream& operator<<(std::ostream& os, const color_t& color)
    {
        os << "(color_t " << static_cast<int>(color.m_value) << ")";
        return os;
    }
};

const inline color_t color_t::black{ 0 };
const inline color_t color_t::red{ 1 };
const inline color_t color_t::green{ 2 };
const inline color_t color_t::yellow{ 3 };
const inline color_t color_t::blue{ 4 };
const inline color_t color_t::magenta{ 5 };
const inline color_t color_t::cyan{ 6 };
const inline color_t color_t::white{ 7 };
const inline color_t color_t::bright_black{ 8 };
const inline color_t color_t::bright_red{ 9 };
const inline color_t color_t::bright_green{ 10 };
const inline color_t color_t::bright_yellow{ 11 };
const inline color_t color_t::bright_blue{ 12 };
const inline color_t color_t::bright_magenta{ 13 };
const inline color_t color_t::bright_cyan{ 14 };
const inline color_t color_t::bright_white{ 15 };

struct font_t
{
    using underlying_type = std::uint16_t;

    underlying_type m_value;

    constexpr explicit font_t(underlying_type value = 0) : m_value(value) { }

    constexpr friend bool operator==(const font_t lhs, const font_t rhs) { return lhs.m_value == rhs.m_value; }
    constexpr friend bool operator!=(const font_t lhs, const font_t rhs) { return !(lhs == rhs); }
    constexpr friend bool operator<(const font_t lhs, const font_t rhs) { return lhs.m_value < rhs.m_value; }
    constexpr friend bool operator>(const font_t lhs, const font_t rhs) { return rhs < lhs; }
    constexpr friend bool operator<=(const font_t lhs, const font_t rhs) { return !(lhs > rhs); }
    constexpr friend bool operator>=(const font_t lhs, const font_t rhs) { return !(lhs < rhs); }

    constexpr friend font_t operator~(const font_t item) { return font_t(~item.m_value); }
    constexpr friend font_t operator&(const font_t lhs, const font_t rhs) { return font_t(lhs.m_value & rhs.m_value); }
    constexpr friend font_t& operator&=(font_t& lhs, const font_t rhs)
    {
        lhs.m_value &= rhs.m_value;
        return lhs;
    }

    constexpr friend font_t operator|(const font_t lhs, const font_t rhs) { return font_t(lhs.m_value | rhs.m_value); }

    constexpr friend font_t& operator|=(font_t& lhs, const font_t rhs)
    {
        lhs.m_value |= rhs.m_value;
        return lhs;
    }

    constexpr friend font_t operator^(const font_t lhs, const font_t rhs) { return font_t(lhs.m_value ^ rhs.m_value); }

    constexpr friend font_t& operator^=(font_t& lhs, const font_t rhs)
    {
        lhs.m_value ^= rhs.m_value;
        return lhs;
    }

    constexpr explicit operator bool() const { return static_cast<bool>(m_value); }

    static const font_t none;
    static const font_t standout;
    static const font_t bold;
    static const font_t dim;
    static const font_t italic;
    static const font_t underlined;
    static const font_t blink;
    static const font_t inverse;
    static const font_t hidden;
    static const font_t crossed_out;
    static const font_t double_underlined;

    bool contains(font_t v) const { return static_cast<bool>(*this & v); }

    font_t& unset(const font_t v) { return *this = *this & ~v; }
    font_t& set(const font_t v) { return *this = *this | v; }

    static const std::vector<std::pair<font_t, std::string_view>>& font_map()
    {
        static const std::vector<std::pair<font_t, std::string_view>> map = {
            { font_t::none, "none" },
            { font_t::standout, "standout" },
            { font_t::bold, "bold" },
            { font_t::dim, "dim" },
            { font_t::italic, "italic" },
            { font_t::underlined, "underlined" },
            { font_t::blink, "blink" },
            { font_t::inverse, "inverse" },
            { font_t::hidden, "hidden" },
            { font_t::crossed_out, "crossed_out" },
            { font_t::double_underlined, "double_underlined" },
        };
        return map;
    }

    friend std::ostream& operator<<(std::ostream& os, font_t item)
    {
        os << "(font_t";
        for (const auto& [f, n] : font_map())
        {
            if (item.contains(f))
            {
                os << " " << n;
            }
        }
        os << ")";
        return os;
    }

    static std::optional<font_t> parse(const std::string& text)
    {
        std::stringstream ss(text);
        std::string token;

        std::optional<font_t> result = {};

        while (std::getline(ss, token, '+'))
        {
            if (const auto iter = std::find_if(
                    font_t::font_map().begin(),
                    font_t::font_map().end(),
                    [&token](const auto& pair) { return pair.second == token; });
                iter != font_t::font_map().end())
            {
                if (result)
                {
                    *result |= iter->first;
                }
                else
                {
                    result = iter->first;
                }
            }
        }
        return result;
    }
};

const inline font_t font_t::none{ 0 };
const inline font_t font_t::standout{ 1 << 0 };
const inline font_t font_t::bold{ 1 << 1 };
const inline font_t font_t::dim{ 1 << 2 };
const inline font_t font_t::italic{ 1 << 3 };
const inline font_t font_t::underlined{ 1 << 4 };
const inline font_t font_t::blink{ 1 << 5 };
const inline font_t font_t::inverse{ 1 << 6 };
const inline font_t font_t::hidden{ 1 << 7 };
const inline font_t font_t::crossed_out{ 1 << 8 };
const inline font_t font_t::double_underlined{ 1 << 9 };

inline escape_sequence_t make_ansi_code(font_t font)
{
    static const std::vector<std::pair<font_t, int>> font_to_code = {
        { font_t::standout, 7 }, { font_t::bold, 1 },       { font_t::dim, 2 },
        { font_t::italic, 3 },   { font_t::underlined, 4 }, { font_t::blink, 5 },
        { font_t::inverse, 7 },  { font_t::hidden, 8 },     { font_t::crossed_out, 9 },
    };
    escape_sequence_t code = {};
    for (const auto& [f, c] : font_to_code)
    {
        if (font.contains(f))
        {
            code.push_back(c);
        }
    }
    return code;
}

struct stream_t
{
    struct state_t
    {
        bool at_line_start = true;
        bool should_add_newline = false;
        std::size_t current_line_indent = 0;
    };

    std::ostream& m_os;
    std::vector<std::size_t> m_indent_levels = { 0 };
    std::vector<std::size_t> m_tab_offsets = { 0 };
    state_t m_state = {};

    explicit stream_t(std::ostream& output) : m_os{ output } { }

    void write_indent(const state_t& current_state) const
    {
        m_os << std::string(current_state.current_line_indent + m_tab_offsets.back(), ' ');
    }

    state_t handle_newline(state_t current_state) const
    {
        if (current_state.should_add_newline)
        {
            m_os << '\n';
            current_state.at_line_start = true;
            current_state.should_add_newline = false;
            current_state.current_line_indent = m_indent_levels.back();
        }
        return current_state;
    }

    state_t write_char(char ch, state_t current_state) const
    {
        current_state = handle_newline(current_state);

        if (current_state.at_line_start && ch != '\n')
        {
            current_state.current_line_indent = m_indent_levels.back();
            write_indent(current_state);
            current_state.at_line_start = false;
        }

        m_os.put(ch);
        if (ch == '\n')
        {
            current_state.at_line_start = true;
            current_state.current_line_indent = m_indent_levels.back();
        }

        return current_state;
    }

    void write(std::string_view text)
    {
        m_state = std::accumulate(
            text.begin(),
            text.end(),
            m_state,
            [this](state_t current_state, char ch) { return write_char(ch, current_state); });
    }

    void write_ansi(const escape_sequence_t& ansi_code)
    {
        m_state = handle_newline(m_state);

        if (m_state.at_line_start)
        {
            m_state.current_line_indent = m_indent_levels.back();
            write_indent(m_state);
            m_state.at_line_start = false;
        }

        m_os << ansi_code;
    }

    void newline() { m_state.should_add_newline = true; }

    void indent(std::size_t spaces_per_level = 2) { m_indent_levels.push_back(m_indent_levels.back() + spaces_per_level); }
    void unindent() { m_indent_levels.pop_back(); }

    void tab(std::size_t spaces) { m_tab_offsets.push_back(m_tab_offsets.back() + spaces); }
    void untab() { m_tab_offsets.pop_back(); }
};

struct node_t
{
    struct impl_t
    {
        virtual ~impl_t() = default;
        virtual void render(stream_t& is) const = 0;
        virtual std::unique_ptr<impl_t> clone() const = 0;
        virtual bool is_list_item() const = 0;
    };

    std::unique_ptr<impl_t> m_impl;

    explicit node_t(std::unique_ptr<impl_t> impl) : m_impl(std::move(impl)) { }

    node_t(const node_t& other) : m_impl(other.m_impl->clone()) { }

    node_t(node_t&&) noexcept = default;

    node_t& operator=(const node_t& other)
    {
        if (this != &other)
        {
            m_impl = other.m_impl->clone();
        }
        return *this;
    }

    node_t& operator=(node_t&&) noexcept = default;

    stream_t& render(stream_t& is) const
    {
        m_impl->render(is);
        return is;
    }

    bool is_list_item() const { return m_impl->is_list_item(); }

    friend stream_t& operator<<(stream_t& stream, const node_t& node)
    {
        node.render(stream);
        return stream;
    }

    friend std::ostream& operator<<(std::ostream& os, const node_t& node)
    {
        stream_t stream(os);
        node.render(stream);
        return os;
    }
};

namespace detail
{

template <class Self, bool IsListItem = false>
struct node_base_t : node_t::impl_t
{
    std::unique_ptr<node_t::impl_t> clone() const override
    {
        return std::make_unique<Self>(static_cast<const Self&>(*this));
    }

    bool is_list_item() const override { return IsListItem; }
};

struct text_node_t : node_base_t<text_node_t>
{
    std::string m_content;

    explicit text_node_t(std::string content) : m_content(std::move(content)) { }

    void render(stream_t& is) const override { is.write(m_content); }
};

struct text_ref_node_t : node_base_t<text_ref_node_t>
{
    std::string_view m_content;

    explicit text_ref_node_t(std::string_view content) : m_content(std::move(content)) { }

    void render(stream_t& is) const override { is.write(m_content); }
};

struct block_node_t : node_base_t<block_node_t>
{
    std::vector<node_t> m_children;

    explicit block_node_t(std::vector<node_t> children) : m_children(std::move(children)) { }

    void render(stream_t& is) const override
    {
        for (const auto& child : m_children)
        {
            child.render(is);
        }
    }
};

struct indented_node_t : node_base_t<indented_node_t>
{
    std::vector<node_t> m_children;

    explicit indented_node_t(std::vector<node_t> children) : m_children(std::move(children)) { }

    void render(stream_t& is) const override
    {
        is.indent();
        for (const auto& child : m_children)
        {
            child.render(is);
        }
        is.unindent();
    }
};

struct line_node_t : node_base_t<line_node_t>
{
    std::vector<node_t> m_children;

    explicit line_node_t(std::vector<node_t> children) : m_children(std::move(children)) { }

    void render(stream_t& is) const override
    {
        for (const auto& child : m_children)
        {
            child.render(is);
        }
        is.newline();
    }
};

struct list_item_node_t : node_base_t<list_item_node_t, true>
{
    std::vector<node_t> m_children;

    explicit list_item_node_t(std::vector<node_t> children) : m_children(std::move(children)) { }

    void render(stream_t& is) const override
    {
        for (const auto& child : m_children)
        {
            child.render(is);
        }
    }
};

struct list_node_t : node_base_t<list_node_t>
{
    std::string m_list_style;
    std::vector<node_t> m_children;

    explicit list_node_t(std::string list_style, std::vector<node_t> children)
        : m_list_style(std::move(list_style))
        , m_children(std::move(children))
    {
        if (!std::all_of(m_children.begin(), m_children.end(), std::mem_fn(&node_t::is_list_item)))
        {
            throw std::invalid_argument{ "All children of a list must be list items" };
        }
    }

    void render(stream_t& is) const override
    {
        for (std::size_t i = 0; i < m_children.size(); ++i)
        {
            if (i != 0)
            {
                is.newline();
            }
            std::string prefix = std::to_string(i + 1) + ". ";
            is.write(prefix);
            is.tab(prefix.length());
            m_children[i].render(is);
            is.untab();
        }
    }
};

struct style_info_t
{
    std::optional<color_t> fg_color = {};
    std::optional<color_t> bg_color = {};
    std::optional<font_t> font = {};

    static std::optional<style_info_t> parse(const std::string& style_name)
    {
        std::stringstream ss(style_name);
        std::string token;

        style_info_t info = {};
        while (std::getline(ss, token, ' '))
        {
            if (token.find("fg:") == 0)
            {
                if (auto maybe_color = color_t::parse(token.substr(3)))
                {
                    info.fg_color = *maybe_color;
                }
            }
            else if (token.find("bg:") == 0)
            {
                if (auto maybe_color = color_t::parse(token.substr(3)))
                {
                    info.bg_color = *maybe_color;
                }
            }
            else if (const auto maybe_font = font_t::parse(token))
            {
                info.font = maybe_font;
            }
            else if (auto maybe_color = color_t::parse(token))
            {
                info.fg_color = *maybe_color;
            }
        }
        if (info.fg_color || info.bg_color || info.font)
        {
            return info;
        }
        return {};
    }

    escape_sequence_t to_ansi_code() const
    {
        escape_sequence_t code = {};
        if (fg_color)
        {
            code += escape_sequence_t{ 38, 5, fg_color->value() };
        }
        if (bg_color)
        {
            code += escape_sequence_t{ 48, 5, bg_color->value() };
        }
        if (font)
        {
            code += make_ansi_code(*font);
        }
        return code;
    }
};

struct styled_node_impl : node_base_t<styled_node_impl>
{
    std::string m_style_name;
    std::vector<node_t> m_children;

    explicit styled_node_impl(std::string style_name, std::vector<node_t> children)
        : m_style_name(std::move(style_name))
        , m_children(std::move(children))
    {
    }

    void render(stream_t& is) const override
    {
        const auto ansi_code = style_info_t::parse(m_style_name);
        if (ansi_code)
        {
            is.write_ansi(ansi_code->to_ansi_code());
        }
        for (const auto& child : m_children)
        {
            child.render(is);
        }
        if (ansi_code)
        {
            is.write_ansi(escape_sequence_t{ 0 });
        }
    }
};

template <class Impl, class... Args>
node_t make_node(Args&&... args)
{
    return node_t{ std::make_unique<Impl>(std::forward<Args>(args)...) };
}

struct create_fn
{
    template <class... Args>
    std::vector<node_t> operator()(Args&&... args) const
    {
        std::vector<node_t> children = {};
        children.reserve(sizeof...(args));
        append(children, std::forward<Args>(args)...);
        return children;
    }

    template <class Head, class... Tail>
    static void append(std::vector<node_t>& v, Head&& head, Tail&&... tail)
    {
        append_item(v, std::forward<Head>(head));
        if constexpr (sizeof...(tail) > 0)
        {
            append(v, std::forward<Tail>(tail)...);
        }
    }

    static void append_item(std::vector<node_t>& v, const char* item) { v.push_back(make_node<text_ref_node_t>(item)); }

    static void append_item(std::vector<node_t>& v, const node_t& item) { v.push_back(item); }

    static void append_item(std::vector<node_t>& v, node_t&& item) { v.push_back(std::move(item)); }

    static void append_item(std::vector<node_t>& v, const std::vector<node_t>& item)
    {
        v.insert(v.end(), item.begin(), item.end());
    }

    static void append_item(std::vector<node_t>& v, std::vector<node_t>&& item)
    {
        v.insert(v.end(), std::make_move_iterator(item.begin()), std::make_move_iterator(item.end()));
    }

    template <class T>
    static void append_item(std::vector<node_t>& v, T&& value)
    {
        std::ostringstream ss;
        ss << value;
        v.push_back(make_node<text_node_t>(ss.str()));
    }
};

static constexpr auto create = create_fn{};

template <class Impl>
struct node_builder_fn
{
    template <class... Args>
    auto operator()(Args&&... args) const -> node_t
    {
        return make_node<Impl>(create(std::forward<Args>(args)...));
    }
};

struct styled_node_builder
{
    std::string m_style_name;

    explicit styled_node_builder(std::string style_name) : m_style_name(std::move(style_name)) { }

    template <class... Args>
    auto operator()(Args&&... args) const -> node_t
    {
        return make_node<styled_node_impl>(m_style_name, create(std::forward<Args>(args)...));
    }
};

}  // namespace detail

constexpr auto indented = detail::node_builder_fn<detail::indented_node_t>{};
constexpr auto line = detail::node_builder_fn<detail::line_node_t>{};

template <class... Args>
auto list(Args&&... args) -> node_t
{
    return detail::make_node<detail::list_node_t>("numbered", detail::create(std::forward<Args>(args)...));
}

constexpr auto list_item = detail::node_builder_fn<detail::list_item_node_t>{};
constexpr auto block = detail::node_builder_fn<detail::block_node_t>{};

inline auto styled(std::string style_name) -> detail::styled_node_builder
{
    return detail::styled_node_builder{ std::move(style_name) };
}

}  // namespace ansi

}  // namespace zx

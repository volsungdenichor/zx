#pragma once

#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <sstream>
#include <vector>

namespace zx
{

namespace ansi
{

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

    void write_ansi(std::string_view ansi_code)
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
            throw std::invalid_argument("All children of a list must be list items");
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
        const std::string ansi_code = parse_ansi_codes(m_style_name);
        if (!ansi_code.empty())
        {
            is.write_ansi(ansi_code);
        }
        for (const auto& child : m_children)
        {
            child.render(is);
        }
        if (!ansi_code.empty())
        {
            is.write_ansi("\033[0m");  // Reset all attributes
        }
    }

    static std::string parse_ansi_codes(const std::string& style_name)
    {
        static const std::map<std::string, int> color_map = { { "black", 0 },         { "red", 1 },
                                                              { "green", 2 },         { "yellow", 3 },
                                                              { "blue", 4 },          { "magenta", 5 },
                                                              { "cyan", 6 },          { "white", 7 },
                                                              { "bright_black", 8 },  { "bright_red", 9 },
                                                              { "bright_green", 10 }, { "bright_yellow", 11 },
                                                              { "bright_blue", 12 },  { "bright_magenta", 13 },
                                                              { "bright_cyan", 14 },  { "bright_white", 15 } };

        static const std::map<std::string, std::string> style_map
            = { { "bold", "1" },  { "dim", "2" },     { "italic", "3" }, { "underlined", "4" },
                { "blink", "5" }, { "reverse", "7" }, { "hidden", "8" }, { "strikethrough", "9" } };

        auto parse_color = [](const std::string& color_str) -> std::optional<int>
        {
            if (color_map.count(color_str))
            {
                return color_map.at(color_str);
            }

            if (color_str.find("0x") == 0 && color_str.length() == 8)
            {
                try
                {
                    unsigned long hex_value = std::stoul(color_str.substr(2), nullptr, 16);
                    int r = (hex_value >> 16) & 0xFF;
                    int g = (hex_value >> 8) & 0xFF;
                    int b = (hex_value >> 0) & 0xFF;

                    int r_idx = (r * 6) / 256;
                    int g_idx = (g * 6) / 256;
                    int b_idx = (b * 6) / 256;

                    return 16 + 36 * r_idx + 6 * g_idx + b_idx;
                }
                catch (...)
                {
                    return std::nullopt;
                }
            }

            if (color_str.find("gray:") == 0 || color_str.find("grey:") == 0)
            {
                try
                {
                    size_t colon_pos = color_str.find(':');
                    int level = std::stoi(color_str.substr(colon_pos + 1));
                    if (level >= 0 && level <= 23)
                    {
                        return 232 + level;
                    }
                }
                catch (...)
                {
                    return std::nullopt;
                }
            }

            return std::nullopt;
        };

        std::vector<std::string> codes;

        std::stringstream ss(style_name);
        std::string token;

        while (std::getline(ss, token, '+'))
        {
            if (token.find("fg:") == 0)
            {
                if (auto color_code = parse_color(token.substr(3)))
                {
                    codes.push_back("38");
                    codes.push_back("5");
                    codes.push_back(std::to_string(*color_code));
                }
            }
            else if (token.find("bg:") == 0)
            {
                if (auto color_code = parse_color(token.substr(3)))
                {
                    codes.push_back("48");
                    codes.push_back("5");
                    codes.push_back(std::to_string(*color_code));
                }
            }
            else if (style_map.count(token))
            {
                codes.push_back(style_map.at(token));
            }
            else
            {
                if (auto color_code = parse_color(token))
                {
                    codes.push_back("38");
                    codes.push_back("5");
                    codes.push_back(std::to_string(*color_code));
                }
            }
        }

        if (codes.empty())
        {
            return "";
        }

        std::string result = "\033[";
        for (size_t i = 0; i < codes.size(); ++i)
        {
            if (i != 0)
            {
                result += ";";
            }
            result += codes[i];
        }
        result += "m";
        return result;
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
    node_t operator()(Args&&... args) const
    {
        return make_node<Impl>(create(std::forward<Args>(args)...));
    }
};

constexpr auto indented = node_builder_fn<indented_node_t>{};
constexpr auto line = node_builder_fn<line_node_t>{};

template <class... Args>
node_t list(Args&&... args)
{
    return make_node<list_node_t>("numbered", create(std::forward<Args>(args)...));
}

constexpr auto list_item = node_builder_fn<list_item_node_t>{};
constexpr auto block = node_builder_fn<block_node_t>{};

struct styled_node_builder
{
    std::string m_style_name;

    explicit styled_node_builder(std::string style_name) : m_style_name(std::move(style_name)) { }

    template <class... Args>
    node_t operator()(Args&&... args) const
    {
        return make_node<styled_node_impl>(m_style_name, create(std::forward<Args>(args)...));
    }
};

inline styled_node_builder styled(std::string style_name)
{
    return styled_node_builder{ std::move(style_name) };
}

}  // namespace ansi

}  // namespace zx

#pragma once

#include <zx/nested_text/value.hpp>

namespace zx
{

namespace nested_text
{

struct pretty_print_options_t
{
    int indent_size = 2;
    std::size_t max_inline_length = 60;
    bool compact_maps = true;
};

namespace detail
{

class pretty_printer_t
{
    std::ostream& m_os;
    const pretty_print_options_t& m_options;
    int m_current_indent = 0;

    pretty_printer_t& write_indent()
    {
        m_os << std::string(static_cast<std::size_t>(m_current_indent), ' ');
        return *this;
    }

    pretty_printer_t& write_newline()
    {
        m_os << "\n";
        return *this;
    }

    static bool is_simple_value(const value_t& item) { return !item.if_list() && !item.if_map(); }

    std::size_t estimate_length(const value_t& item) const
    {
        std::ostringstream temp;
        temp << item;
        return temp.str().length();
    }

    template <class T>
    static bool is_compact(const T& item)
    {
        return item.size() <= 3 && std::all_of(item.begin(), item.end(), is_simple_value);
    }

    void print(const list_t& item, bool inline_mode)
    {
        m_os << "[";

        if (item.empty())
        {
            m_os << "]";
            return;
        }

        const bool should_inline = inline_mode || is_compact(item);

        if (should_inline && estimate_length(value_t(item)) < m_options.max_inline_length)
        {
            for (auto it = item.begin(); it != item.end(); ++it)
            {
                if (it != item.begin())
                {
                    m_os << " ";
                }
                m_os << *it;
            }
        }
        else
        {
            m_current_indent += m_options.indent_size;
            for (auto it = item.begin(); it != item.end(); ++it)
            {
                write_newline().write_indent();
                print_value(*it, false);
            }
            m_current_indent -= m_options.indent_size;
            write_newline().write_indent();
        }

        m_os << "]";
    }

    void print(const map_t& item, bool inline_mode)
    {
        m_os << "{";

        if (item.empty())
        {
            m_os << "}";
            return;
        }

        const bool should_inline
            = m_options.compact_maps && inline_mode && item.size() <= 2
              && std::all_of(item.begin(), item.end(), [](const auto& p) { return is_simple_value(p.second); });

        if (should_inline)
        {
            for (auto it = item.begin(); it != item.end(); ++it)
            {
                if (it != item.begin())
                {
                    m_os << " ";
                }
                m_os << ":" << it->first << " " << it->second;
            }
        }
        else
        {
            const int indent_increment = m_options.compact_maps ? 2 : m_options.indent_size;
            m_current_indent += indent_increment;
            for (auto it = item.begin(); it != item.end(); ++it)
            {
                write_newline().write_indent();
                m_os << ":" << it->first << " ";
                print_value(it->second, true);
            }
            m_current_indent -= indent_increment;
            write_newline().write_indent();
        }

        m_os << "}";
    }

    void print_value(const value_t& item, bool inline_mode)
    {
        if (const auto maybe_list = item.if_list())
        {
            print(*maybe_list, inline_mode);
        }
        else if (const auto maybe_map = item.if_map())
        {
            print(*maybe_map, inline_mode);
        }
        else
        {
            m_os << item;
        }
    }

public:
    pretty_printer_t(std::ostream& os, const pretty_print_options_t& options) : m_os{ os }, m_options{ options } { }

    std::ostream& operator()(const value_t& item)
    {
        print_value(item, false);
        write_newline();
        return m_os;
    }
};

}  // namespace detail

inline void pretty_print(std::ostream& os, const value_t& item, const pretty_print_options_t& options = {})
{
    detail::pretty_printer_t printer(os, options);
    printer(item);
}

inline std::string to_pretty_string(const value_t& item, const pretty_print_options_t& options = {})
{
    std::ostringstream ss;
    pretty_print(ss, item, options);
    return ss.str();
}

}  // namespace nested_text

}  // namespace zx

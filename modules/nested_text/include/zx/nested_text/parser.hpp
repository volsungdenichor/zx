#pragma once

#include <stdexcept>
#include <zx/nested_text/node.hpp>

namespace zx
{
namespace nested_text
{

struct location_t
{
    std::size_t line;
    std::size_t column;

    friend std::ostream& operator<<(std::ostream& os, const location_t& loc)
    {
        return os << "line " << (loc.line + 1) << ", column " << (loc.column + 1);
    }
};

class parse_error : public std::runtime_error
{
public:
    location_t location;

    parse_error(const std::string& message, location_t loc) : std::runtime_error(format_message(message, loc)), location(loc)
    {
    }

private:
    static std::string format_message(const std::string& message, location_t loc)
    {
        std::ostringstream ss;
        ss << "Parse error at " << loc << ": " << message;
        return ss.str();
    }
};

namespace detail
{

struct char_t
{
    char value;
    location_t location;
};

class stream_t
{
    std::string_view m_content;
    std::size_t m_pos = 0;
    location_t m_location = { 0, 0 };

public:
    stream_t(std::string_view content) : m_content(content) { }

    bool eof() const { return m_pos >= m_content.size(); }

    char_t peek() const
    {
        if (eof())
        {
            throw parse_error{ "Unexpected end of input", m_location };
        }
        return { m_content[m_pos], m_location };
    }

    char_t get()
    {
        auto result = peek();
        m_pos++;

        if (result.value == '\n')
        {
            m_location.line++;
            m_location.column = 0;
        }
        else
        {
            m_location.column++;
        }

        return result;
    }

    location_t location() const { return m_location; }

    void skip_whitespace_and_comments()
    {
        while (!eof())
        {
            const char ch = m_content[m_pos];

            if (is_space(ch) || ch == ',')
            {
                get();
            }
            else if (ch == ';')
            {
                while (!eof() && m_content[m_pos] != '\n')
                {
                    get();
                }
            }
            else
            {
                break;
            }
        }
    }
};

class parser_t
{
    stream_t& m_stream;

    bool is_delimiter(char ch) const
    {
        static const std::string delimiters = "[]{};,\"\\:";
        return is_space(ch) || delimiters.find(ch) != std::string::npos;
    }

    std::tuple<std::string, location_t> read_token()
    {
        const location_t start_loc = m_stream.location();
        std::string token = {};

        while (!m_stream.eof())
        {
            char ch = m_stream.peek().value;
            if (is_delimiter(ch))
            {
                break;
            }
            token += m_stream.get().value;
        }

        return { std::move(token), start_loc };
    }

    node_t parse_string()
    {
        const location_t start_loc = m_stream.location();
        m_stream.get();

        std::string result;

        while (!m_stream.eof())
        {
            const char_t ch = m_stream.get();

            if (ch.value == '"')
            {
                return string_t{ result };
            }
            else if (ch.value == '\\')
            {
                if (m_stream.eof())
                {
                    throw parse_error{ "Unexpected end of string", m_stream.location() };
                }

                const char_t escape = m_stream.get();
                switch (escape.value)
                {
                    case 'n': result += '\n'; break;
                    case 'r': result += '\r'; break;
                    case 't': result += '\t'; break;
                    case '\\': result += '\\'; break;
                    case '"': result += '"'; break;
                    default: throw parse_error{ std::string("Invalid escape sequence: \\") + escape.value, escape.location };
                }
            }
            else
            {
                result += ch.value;
            }
        }

        throw parse_error{ "Unterminated string", start_loc };
    }

    string_t parse_keyword()
    {
        const location_t start_loc = m_stream.location();
        m_stream.get();

        string_t name = {};
        while (!m_stream.eof() && !is_delimiter(m_stream.peek().value))
        {
            name += m_stream.get().value;
        }

        if (name.empty())
        {
            throw parse_error{ "Empty keyword", start_loc };
        }
        return name;
    }

    template <class T>
    std::pair<T, location_t> parse_collection(char open_delim, char close_delim, const std::string& error_message)
    {
        (void)open_delim;
        const location_t start_loc = m_stream.location();
        m_stream.get();  // consume '('

        T result = {};

        while (true)
        {
            m_stream.skip_whitespace_and_comments();

            if (m_stream.eof())
            {
                throw parse_error{ error_message, start_loc };
            }

            if (m_stream.peek().value == close_delim)
            {
                m_stream.get();
                return { std::move(result), start_loc };
            }

            result.push_back(parse_value());
        }
    }

    node_t parse_list() { return parse_collection<list_t>('[', ']', "Unterminated list").first; }

    node_t parse_map()
    {
        const location_t start_loc = m_stream.location();
        m_stream.get();

        map_t result = {};

        while (true)
        {
            m_stream.skip_whitespace_and_comments();

            if (m_stream.eof())
            {
                throw parse_error{ "Unterminated map", start_loc };
            }

            if (m_stream.peek().value == '}')
            {
                m_stream.get();
                return result;
            }

            if (m_stream.peek().value != ':')
            {
                throw parse_error{ "Map keys must be keywords", m_stream.location() };
            }

            string_t key = parse_keyword();

            m_stream.skip_whitespace_and_comments();
            if (m_stream.eof() || m_stream.peek().value == '}')
            {
                throw parse_error{ "Map requires an even number of elements", start_loc };
            }

            result[std::move(key)] = parse_value();
        }
    }

public:
    parser_t(stream_t& stream) : m_stream(stream) { }

    node_t parse_value()
    {
        using parse_fn = node_t (parser_t::*)();

        static const std::vector<std::tuple<char, parse_fn>> parsers = {
            { '"', &parser_t::parse_string },
            { '[', &parser_t::parse_list },
            { '{', &parser_t::parse_map },
        };

        m_stream.skip_whitespace_and_comments();

        if (m_stream.eof())
        {
            throw parse_error{ "Unexpected end of input", m_stream.location() };
        }

        const char ch = m_stream.peek().value;

        for (const auto& [identifier, fn] : parsers)
        {
            if (ch == identifier)
            {
                return (this->*fn)();
            }
        }

        if (ch == ')' || ch == ']' || ch == '}')
        {
            throw parse_error{ std::string("Unexpected closing delimiter: ") + ch, m_stream.location() };
        }

        if (is_delimiter(ch))
        {
            throw parse_error{ std::string("Unexpected delimiter: ") + ch, m_stream.location() };
        }

        auto [token, start_loc] = read_token();
        (void)start_loc;
        return token;
    }
};

struct parse_fn
{
    node_t operator()(std::string_view text) const
    {
        std::vector<node_t> values = read_values(text);
        if (values.empty())
        {
            return node_t{};
        }
        else if (values.size() == 1)
        {
            return values[0];
        }
        else
        {
            list_t result = {};
            result.insert(result.end(), values.begin(), values.end());
            return result;
        }
    }

    static std::vector<node_t> read_values(std::string_view text)
    {
        stream_t stream(text);
        parser_t parser(stream);

        std::vector<node_t> values = {};

        while (true)
        {
            stream.skip_whitespace_and_comments();
            if (stream.eof())
            {
                break;
            }
            values.push_back(parser.parse_value());
        }

        return values;
    }
};

}  // namespace detail

constexpr inline auto parse = detail::parse_fn{};

}  // namespace nested_text
}  // namespace zx

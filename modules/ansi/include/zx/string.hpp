#pragma once

#include <array>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace zx
{
namespace ansi
{

namespace detail
{

struct utf_value_t
{
    using size_type = unsigned char;
    using const_iterator = const char*;
    std::array<char, 4> m_bytes;
    size_type m_size;

    template <class... Args>
    utf_value_t(Args&&... args) : m_bytes{ static_cast<char>(std::forward<Args>(args))... }
                                , m_size(sizeof...(args))
    {
        static_assert(sizeof...(args) <= 4, "UTF-8 encoding can be at most 4 bytes long");
    }

    constexpr size_type size() const noexcept { return m_size; }
    constexpr const_iterator begin() const noexcept { return m_bytes.data(); }
    constexpr const_iterator end() const noexcept { return begin() + size(); }

    friend std::ostream& operator<<(std::ostream& os, const utf_value_t& value)
    {
        os.write(value.begin(), value.size());
        return os;
    }
};

inline utf_value_t utf32_to_utf8(char32_t ch)
{
    if (ch < 0x80)
    {
        return utf_value_t{ ch };
    }
    else if (ch < 0x800)
    {
        return utf_value_t{ 0xC0 | (ch >> 6), 0x80 | (ch & 0x3F) };
    }
    else if (ch < 0x10000)
    {
        return utf_value_t{ 0xE0 | (ch >> 12), 0x80 | ((ch >> 6) & 0x3F), 0x80 | (ch & 0x3F) };
    }
    else if (ch < 0x110000)
    {
        return utf_value_t{ 0xF0 | (ch >> 18), 0x80 | ((ch >> 12) & 0x3F), 0x80 | ((ch >> 6) & 0x3F), 0x80 | (ch & 0x3F) };
    }

    throw std::invalid_argument{ "Invalid UTF-32 code point" };
}

inline std::pair<char32_t, std::string_view> utf8_to_utf32(std::string_view s)
{
    if (s.empty())
    {
        throw std::invalid_argument{ "Input string is empty" };
    }

    unsigned char c0 = static_cast<unsigned char>(s[0]);

    if (c0 < 0x80)
    {
        return { c0, s.substr(1) };
    }

    if ((c0 & 0xE0) == 0xC0)
    {
        if (s.size() < 2)
        {
            throw std::invalid_argument{ "Invalid UTF-8 sequence: expected 2 bytes, but got fewer" };
        }
        if ((s[1] & 0xC0) != 0x80)
        {
            throw std::invalid_argument{ "Invalid UTF-8 sequence: expected continuation byte, but got invalid byte" };
        }

        char32_t c32 = static_cast<char32_t>(((c0 & 0x1F) << 6) | (s[1] & 0x3F));
        return { c32, s.substr(2) };
    }

    if ((c0 & 0xF0) == 0xE0)
    {
        if (s.size() < 3)
        {
            throw std::invalid_argument{ "Invalid UTF-8 sequence: expected 3 bytes, but got fewer" };
        }
        if ((s[1] & 0xC0) != 0x80 || (s[2] & 0xC0) != 0x80)
        {
            throw std::invalid_argument{ "Invalid UTF-8 sequence: expected continuation bytes, but got invalid bytes" };
        }
        char32_t c32 = static_cast<char32_t>(((c0 & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F));
        return { c32, s.substr(3) };
    }

    if ((c0 & 0xF8) == 0xF0)
    {
        if (s.size() < 4)
        {
            throw std::invalid_argument{ "Invalid UTF-8 sequence: expected 4 bytes, but got fewer" };
        }
        if ((s[1] & 0xC0) != 0x80 || (s[2] & 0xC0) != 0x80 || (s[3] & 0xC0) != 0x80)
        {
            throw std::invalid_argument{ "Invalid UTF-8 sequence: expected continuation bytes, but got invalid bytes" };
        }
        char32_t c32
            = static_cast<char32_t>(((c0 & 0x07) << 18) | ((s[1] & 0x3F) << 12) | ((s[2] & 0x3F) << 6) | (s[3] & 0x3F));
        return { c32, s.substr(4) };
    }

    throw std::invalid_argument{ "Invalid UTF-8 sequence: unexpected byte" };
}

}  // namespace detail

struct glyph_t
{
    char32_t m_data;

    glyph_t() = default;

    explicit glyph_t(char32_t data) : m_data(data) { }

    explicit glyph_t(std::string_view txt) : glyph_t()
    {
        auto [value, remainder] = detail::utf8_to_utf32(txt);
        if (!remainder.empty())
        {
            throw std::invalid_argument{ "Input string contains more than one UTF-8 character: '" + std::string(txt) + "'" };
        }
        m_data = value;
    }

    explicit glyph_t(const char* txt) : glyph_t(std::string_view(txt)) { }

    explicit glyph_t(char ch) : glyph_t(std::string_view(&ch, 1)) { }

    friend bool operator==(const glyph_t& lhs, const glyph_t& rhs) { return lhs.m_data == rhs.m_data; }
    friend bool operator!=(const glyph_t& lhs, const glyph_t& rhs) { return !(lhs == rhs); }
    friend bool operator<(const glyph_t& lhs, const glyph_t& rhs) { return lhs.m_data < rhs.m_data; }
    friend bool operator>(const glyph_t& lhs, const glyph_t& rhs) { return rhs < lhs; }
    friend bool operator<=(const glyph_t& lhs, const glyph_t& rhs) { return !(lhs > rhs); }
    friend bool operator>=(const glyph_t& lhs, const glyph_t& rhs) { return !(lhs < rhs); }

    friend std::ostream& operator<<(std::ostream& os, const glyph_t& item)
    {
        return os << detail::utf32_to_utf8(item.m_data);
    }
};

struct string_t : public std::vector<glyph_t>
{
    using base_t = std::vector<glyph_t>;
    using base_t::base_t;

    string_t(std::string_view txt) : base_t{}
    {
        this->reserve(txt.size());
        while (!txt.empty())
        {
            auto [glyph, remainder] = detail::utf8_to_utf32(txt);
            this->emplace_back(glyph);
            txt = remainder;
        }
    }

    string_t(const char* txt) : string_t(std::string_view(txt)) { }

    friend std::ostream& operator<<(std::ostream& os, const string_t& item)
    {
        std::copy(item.begin(), item.end(), std::ostream_iterator<glyph_t>(os));
        return os;
    }

    string_t& operator+=(const string_t& other)
    {
        this->reserve(this->size() + other.size());
        this->insert(this->end(), other.begin(), other.end());
        return *this;
    }

    string_t operator+(const string_t& other) const { return string_t{ *this } += other; }
};

}  // namespace ansi
}  // namespace zx

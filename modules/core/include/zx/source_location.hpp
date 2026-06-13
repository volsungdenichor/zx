#pragma once

#include <iostream>
#include <string_view>

namespace zx
{
struct source_location_t
{
    source_location_t(std::size_t line, std::string_view file_name, std::string_view function_name)
        : m_line(line)
        , m_file_name(file_name)
        , m_function_name(function_name)
    {
    }

    source_location_t() = default;

    explicit operator bool() const noexcept { return m_line != 0 && !m_file_name.empty() && !m_function_name.empty(); }

    std::size_t line() const noexcept { return m_line; }
    std::string_view file_name() const noexcept { return m_file_name; }
    std::string_view function_name() const noexcept { return m_function_name; }

    friend std::ostream& operator<<(std::ostream& os, const source_location_t& item)
    {
        return os << item.file_name() << ":" << item.line() << " in " << item.function_name();
    }

private:
    std::size_t m_line;
    std::string_view m_file_name;
    std::string_view m_function_name;
};
}  // namespace zx

#define ZX_SOURCE_LOCATION_CURRENT() zx::source_location_t(__LINE__, __FILE__, __FUNCTION__)

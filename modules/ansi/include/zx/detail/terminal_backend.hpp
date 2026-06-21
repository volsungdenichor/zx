#pragma once

#include <atomic>
#include <memory>
#ifdef _WIN32
#include <zx/detail/terminal_backend_windows.hpp>
#else
#include <zx/detail/terminal_backend_posix.hpp>
#endif

namespace zx
{
namespace ansi
{
namespace detail
{

inline std::unique_ptr<terminal_backend_t> create_terminal_backend()
{
#ifdef _WIN32
    return std::make_unique<windows_terminal_backend_t>();
#else
    return std::make_unique<posix_terminal_backend_t>();
#endif
}

}  // namespace detail
}  // namespace ansi
}  // namespace zx

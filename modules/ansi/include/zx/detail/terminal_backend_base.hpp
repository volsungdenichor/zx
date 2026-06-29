#pragma once

#include <atomic>
#include <cstddef>
#include <optional>
#include <tuple>
#include <zx/event.hpp>

namespace zx
{
namespace ansi
{
namespace detail
{

struct mouse_reporting_options_t
{
    bool button = true;
    bool drag = true;
    bool motion = false;
    bool sgr = true;
};

struct terminal_setup_options_t
{
    bool use_alt_screen = true;
    bool hide_cursor = false;
    mouse_reporting_options_t mouse = {};
};

class terminal_backend_t
{
public:
    virtual ~terminal_backend_t() = default;

    virtual void setup(const terminal_setup_options_t& options) = 0;
    virtual void set_mouse_reporting(const mouse_reporting_options_t& options) = 0;
    virtual void cleanup(bool hide_cursor) = 0;
    virtual int wait_for_input(int tick_ms) const = 0;
    virtual bool should_retry_wait(int wait_result) const = 0;
    virtual mat::vector_t<2, std::ptrdiff_t> get_terminal_size() const = 0;
    virtual std::optional<event_t> read_event(std::atomic<bool>& resize_pending) = 0;
    virtual void write_escape(const char* seq) const = 0;
    virtual void write_output(const char* data, std::size_t size) const = 0;
};

}  // namespace detail
}  // namespace ansi
}  // namespace zx

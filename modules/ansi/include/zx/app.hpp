#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <variant>
#include <vector>
#include <zx/detail/terminal_backend.hpp>
#include <zx/message_bus.hpp>
#include <zx/surface.hpp>
#include <zx/widget.hpp>

namespace zx
{
namespace ansi
{

namespace detail
{
inline std::atomic<bool>& quit_flag()
{
    static std::atomic<bool> flag{ false };
    return flag;
}
}  // namespace detail

using detail::mouse_reporting_options_t;

struct app_options
{
    bool use_alt_screen = true;
    bool hide_cursor = false;
    int tick_ms = 0;
    mouse_reporting_options_t mouse = {};
};

class app_t
{
public:
    explicit app_t(widget_t root, app_options opts = {})
        : m_bus{}
        , m_root(std::move(root))
        , m_opts(opts)
        , m_canvas({ 0, 0 })
        , m_prev_canvas({ 0, 0 })
        , m_cursor_pos{}
        , m_terminal{ detail::create_terminal_backend() }
    {
        m_bus.set_route_builder(
            [this](message_bus_t::subscriber_id_type subscriber_id) -> maybe_t<message_bus_t::subscriber_id_type>
            {
                if (auto w = m_root[subscriber_id])
                {
                    if (auto p = w->parent())
                    {
                        return p->id();
                    }
                }
                return none;
            });
    }

    ~app_t() { cleanup_terminal(); }

    void run()
    {
        setup_terminal();

        const auto size = get_terminal_size();
        m_canvas = surface_t{ size };
        m_prev_canvas = surface_t{ extent_t{} };

        m_running = true;
        detail::quit_flag().store(false);

        render();

        while (m_running)
        {
            int ret = wait_for_input();

            if (ret < 0)
            {
                if (m_terminal->should_retry_wait(ret))
                {
                    ret = 0;
                }
                else
                {
                    break;
                }
            }

            if (ret == 0)
            {
                m_bus.publish(tick_event_t{});
            }
            else
            {
                if (auto ev = read_event(); ev)
                {
                    if (auto e = std::get_if<key_event_t>(&*ev))
                    {
                        m_bus.publish_to(m_root.id(), *e);
                    }
                    if (auto e = std::get_if<mouse_event_t>(&*ev))
                    {
                        m_bus.publish_to(m_root.id(), *e);
                    }
                    if (auto e = std::get_if<resize_event_t>(&*ev))
                    {
                        m_bus.publish_to(m_root.id(), *e);
                    }

                    if (auto event = std::get_if<quit_event_t>(&*ev))
                    {
                        detail::quit_flag().store(true);
                    }
                }
            }

            if (s_resize_pending.exchange(false))
            {
                const auto size = get_terminal_size();
                m_canvas = surface_t{ size };
                m_prev_canvas = surface_t{ extent_t{} };
                m_bus.publish(resize_event_t{ size });
            }

            render();

            if (detail::quit_flag().load())
            {
                m_running = false;
            }
        }

        cleanup_terminal();
    }

    void quit() { m_running = false; }

    void set_mouse_reporting(const mouse_reporting_options_t& options)
    {
        m_opts.mouse = options;
        if (m_running)
        {
            m_terminal->set_mouse_reporting(m_opts.mouse);
        }
    }

    void subscribe(message_bus_t::subscription_info_t info) { m_bus.subscribe(std::move(info)); }

private:
    void setup_terminal()
    {
        m_terminal->setup(detail::terminal_setup_options_t{ m_opts.use_alt_screen, m_opts.hide_cursor, m_opts.mouse });
    }

    void cleanup_terminal() { m_terminal->cleanup(m_opts.hide_cursor); }

    extent_t get_terminal_size() const { return m_terminal->get_terminal_size(); }

    int wait_for_input() const { return m_terminal->wait_for_input(m_opts.tick_ms); }

    std::optional<event_t> read_event() { return m_terminal->read_event(s_resize_pending); }

    void render()
    {
        m_canvas.mut_view().fill(cell_t{});
        m_cursor_pos = std::nullopt;
        m_root.render(m_canvas.mut_view());

        std::string frame = render_diff(m_prev_canvas, m_canvas);

        if (m_cursor_pos)
        {
            const auto& pos = *m_cursor_pos;
            frame += "\033[" + std::to_string(pos[0] + 1) + ";" + std::to_string(pos[1] + 1) + "H";
            frame += "\033[?25h";
        }
        else if (m_opts.hide_cursor)
        {
            frame += "\033[?25l";
        }

        if (!frame.empty())
        {
            m_terminal->write_output(frame.data(), frame.size());
        }

        std::swap(m_canvas, m_prev_canvas);
    }

private:
    message_bus_t m_bus;
    widget_t m_root;
    app_options m_opts;
    surface_t m_canvas;
    surface_t m_prev_canvas;
    std::optional<surface_t::location_type> m_cursor_pos = {};
    std::unique_ptr<detail::terminal_backend_t> m_terminal = {};
    bool m_running = false;

    static std::atomic<bool> s_resize_pending;
};

inline std::atomic<bool> app_t::s_resize_pending{ false };

}  // namespace ansi
}  // namespace zx

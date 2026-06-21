#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <variant>
#include <vector>
#include <zx/detail/terminal_backend.hpp>
#include <zx/surface.hpp>

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

struct app_options
{
    bool use_alt_screen = true;
    bool hide_cursor = false;
    int tick_ms = 0;
};

class app_t
{
public:
    using event_handler_t = std::function<void(const event_t&)>;
    using render_handler_t = std::function<void(surface_t::mut_view_type)>;

    explicit app_t(event_handler_t on_event = {}, render_handler_t on_render = {}, app_options opts = {})
        : m_on_event(on_event)
        , m_on_render(on_render)
        , m_opts(opts)
        , m_canvas({ 0, 0 })
        , m_prev_canvas({ 0, 0 })
        , m_cursor_pos{}
        , m_terminal{ detail::create_terminal_backend() }
    {
    }

    ~app_t() { cleanup_terminal(); }

    void run()
    {
        setup_terminal();

        const auto size = get_terminal_size();
        m_canvas = surface_t{ size };
        m_prev_canvas = surface_t{ surface_t::size_type{} };

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
                m_on_event(tick_event_t{});
            }
            else
            {
                if (auto ev = read_event(); ev)
                {
                    m_on_event(*ev);
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
                m_prev_canvas = surface_t{ surface_t::size_type{} };
                m_on_event(resize_event_t{ size });
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

private:
    void setup_terminal() { m_terminal->setup(m_opts.use_alt_screen, m_opts.hide_cursor); }

    void cleanup_terminal() { m_terminal->cleanup(m_opts.hide_cursor); }

    surface_t::size_type get_terminal_size() const { return m_terminal->get_terminal_size(); }

    int wait_for_input() const { return m_terminal->wait_for_input(m_opts.tick_ms); }

    std::optional<event_t> read_event() { return m_terminal->read_event(s_resize_pending); }

    void render()
    {
        m_canvas.mut_view().fill(cell_t{});
        m_cursor_pos = std::nullopt;
        m_on_render(m_canvas.mut_view());

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
        m_canvas = surface_t{ m_prev_canvas.size() };
    }

private:
    event_handler_t m_on_event;
    render_handler_t m_on_render;
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

#pragma once

#include <algorithm>
#include <utility>
#include <vector>
#include <zx/event.hpp>
#include <zx/message_bus.hpp>
#include <zx/widget.hpp>

namespace zx
{
namespace ansi
{
namespace widgets
{

struct input_fn
{
    struct on_submit_t
    {
        string_t value;
    };

    struct on_change_t
    {
        string_t value;
    };

    struct config_t
    {
        string_t placeholder;
        std::size_t max_length = 0;
        style_t style = {};
        style_t focused_style = {};
        style_t placeholder_style = {};
        style_t cursor_style = {};

        config_t()
        {
            focused_style.font = font_t::underlined;
            placeholder_style.font = font_t::dim;
            cursor_style.font = font_t::inverse;
        }
    };

    class model_t : public widget_t::interface_t
    {
    public:
        explicit model_t(config_t cfg) : m_cfg(std::move(cfg)) { }

        const string_t& value() const { return m_value; }

        extent_t preferred_size() const override
        {
            return { 1, std::max<mat::extent_base_t>(20, static_cast<mat::extent_base_t>(m_cfg.placeholder.size())) };
        }

        bool is_focused() const override { return m_focused; }

        void set_focused(bool focused) override { m_focused = focused; }

        void render(surface_t::mut_view_type view) const override
        {
            const extent_t size = view.extent();
            if (size[0] <= 0 || size[1] <= 0)
            {
                return;
            }

            m_last_lower = mat::lower(view.bounds());
            m_last_extent = size;
            m_has_last_bounds = true;

            const style_t& active_style = m_focused ? m_cfg.focused_style : m_cfg.style;
            view.fill(cell_t{ code_point_t(' '), active_style });

            const bool show_placeholder = m_value.empty() && !m_cfg.placeholder.empty();
            const std::size_t visible_width = static_cast<std::size_t>(size[1]);

            if (show_placeholder)
            {
                render_text(view, m_cfg.placeholder, 0, visible_width, m_cfg.placeholder_style);
            }
            else
            {
                const std::size_t scroll = compute_scroll(visible_width);
                render_text(view, m_value, scroll, visible_width, active_style);
            }

            if (m_focused)
            {
                const std::size_t scroll = compute_scroll(visible_width);
                const std::ptrdiff_t cursor_x = static_cast<std::ptrdiff_t>(m_cursor) - static_cast<std::ptrdiff_t>(scroll);
                if (cursor_x >= 0 && cursor_x < size[1])
                {
                    const location_t pos{ 0, cursor_x };
                    const cell_t old = view[pos];
                    view[pos] = cell_t{ old.symbol, m_cfg.cursor_style };
                }
            }
        }

        void on_attach(message_bus_t& bus) override
        {
            m_bus = &bus;
            m_subscription_ids.clear();

            auto self = subscriber_proxy_t{ id() };

            m_subscription_ids.push_back(
                bus.subscribe(self.on_target<key_event_t>([this](const key_event_t& event) { handle_key(event); })));
            m_subscription_ids.push_back(
                bus.subscribe(self.on_target<mouse_event_t>([this](const mouse_event_t& event) { handle_mouse(event); })));
        }

        void on_detach(message_bus_t& bus) override
        {
            for (auto id : m_subscription_ids)
            {
                bus.unsubscribe(id);
            }
            m_subscription_ids.clear();
            m_bus = nullptr;
        }

    private:
        void handle_key(const key_event_t& event)
        {
            if (!m_focused)
            {
                return;
            }

            if (const key_t* special = event.special())
            {
                switch (*special)
                {
                    case key_t::enter:
                        if (m_bus)
                        {
                            m_bus->publish(on_submit_t{ m_value });
                        }
                        return;
                    case key_t::escape: m_focused = false; return;
                    case key_t::backspace:
                        if (m_cursor > 0)
                        {
                            m_value.erase(m_value.begin() + static_cast<std::ptrdiff_t>(m_cursor - 1));
                            --m_cursor;
                            publish_change();
                        }
                        return;
                    case key_t::del:
                        if (m_cursor < m_value.size())
                        {
                            m_value.erase(m_value.begin() + static_cast<std::ptrdiff_t>(m_cursor));
                            publish_change();
                        }
                        return;
                    case key_t::left:
                        if (m_cursor > 0)
                        {
                            --m_cursor;
                        }
                        return;
                    case key_t::right:
                        if (m_cursor < m_value.size())
                        {
                            ++m_cursor;
                        }
                        return;
                    case key_t::home: m_cursor = 0; return;
                    case key_t::end: m_cursor = m_value.size(); return;
                    default: return;
                }
            }

            if (event.is_printable() && !has(event.modifiers, key_modifiers_t::ctrl)
                && !has(event.modifiers, key_modifiers_t::alt))
            {
                if (m_cfg.max_length == 0 || m_value.size() < m_cfg.max_length)
                {
                    m_value.insert(m_value.begin() + static_cast<std::ptrdiff_t>(m_cursor), *event.code_point());
                    ++m_cursor;
                    publish_change();
                }
            }
        }

        void handle_mouse(const mouse_event_t& event)
        {
            if (!m_has_last_bounds)
            {
                return;
            }

            const auto lower = m_last_lower;
            const auto upper = m_last_lower + m_last_extent - location_t::ones();
            const auto& pos = event.location;
            const bool inside = lower[0] <= pos[0] && pos[0] <= upper[0] && lower[1] <= pos[1] && pos[1] <= upper[1];

            const bool is_left_down = event.kind == mouse_event_kind_t::down && event.button == mouse_button_t::left;
            const bool is_left_up_or_legacy_release = event.kind == mouse_event_kind_t::up;

            if (is_left_down || is_left_up_or_legacy_release)
            {
                if (inside)
                {
                    m_focused = true;

                    const std::size_t visible_width = static_cast<std::size_t>(m_last_extent[1]);
                    const std::size_t scroll = compute_scroll(visible_width);
                    const std::ptrdiff_t local_x = std::max<std::ptrdiff_t>(
                        0, std::min<std::ptrdiff_t>(static_cast<std::ptrdiff_t>(visible_width), pos[1] - lower[1]));
                    m_cursor = std::min(m_value.size(), scroll + static_cast<std::size_t>(local_x));
                }
                else
                {
                    m_focused = false;
                }
            }
        }

        void publish_change()
        {
            if (m_bus)
            {
                m_bus->publish(on_change_t{ m_value });
            }
        }

        static void render_text(
            surface_t::mut_view_type view, const string_t& text, std::size_t offset, std::size_t width, const style_t& style)
        {
            if (offset >= text.size())
            {
                return;
            }

            const std::size_t count = std::min(width, text.size() - offset);
            for (std::size_t i = 0; i < count; ++i)
            {
                view[{ 0, static_cast<mat::location_base_t>(i) }] = cell_t{ text[offset + i], style };
            }
        }

        std::size_t compute_scroll(std::size_t visible_width) const
        {
            if (m_cursor < visible_width)
            {
                return 0;
            }
            return m_cursor - visible_width + 1;
        }

        config_t m_cfg;
        string_t m_value;
        std::size_t m_cursor = 0;
        bool m_focused = false;
        mutable bool m_has_last_bounds = false;
        mutable location_t m_last_lower = {};
        mutable extent_t m_last_extent = {};
        message_bus_t* m_bus = nullptr;
        std::vector<message_bus_t::subscription_id_type> m_subscription_ids;
    };

    inline widget_t operator()(config_t cfg) const { return widget_t::make<model_t>(std::move(cfg)); }

    inline widget_t operator()() const { return (*this)(config_t{}); }
};

constexpr auto input = input_fn{};

}  // namespace widgets
}  // namespace ansi
}  // namespace zx
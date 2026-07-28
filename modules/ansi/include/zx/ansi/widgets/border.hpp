#pragma once

#include <utility>
#include <zx/event.hpp>
#include <zx/message_bus.hpp>
#include <zx/widget.hpp>

namespace zx
{
namespace ansi
{
namespace widgets
{

struct border_fn
{
    struct config_t
    {
        box_style_t box_style = {};
        style_t style = style_t{};
    };

    class model_t : public widget_t::interface_t
    {
    public:
        explicit model_t(widget_t child, config_t cfg) : m_child(std::move(child)), m_cfg(std::move(cfg)) { }

        extent_t preferred_size() const override { return m_child.preferred_size() + extent_t::ones() * 2; }

        bool is_focused() const override { return m_child.m_impl->is_focused(); }

        void set_focused(bool focused) override { m_child.m_impl->set_focused(focused); }

        void render(surface_t::mut_view_type view) const override
        {
            const extent_t size = view.extent();
            if (size[0] < 2 || size[1] < 2)
            {
                m_has_last_inner = false;
                return;
            }

            draw_border(view, view.bounds(), m_cfg.box_style, m_cfg.style);

            const auto inner_lower = mat::lower(view.bounds()) + location_t::ones();

            const extent_t inner_extent = size - extent_t::ones() * 2;
            const auto inner_bounds = mat::box::from_lower_extent(inner_lower, inner_extent);
            m_last_inner_lower = inner_lower;
            m_last_inner_extent = inner_extent;
            m_has_last_inner = true;
            m_child.render(view.slice(mat::to_slice(inner_bounds)));
        }

        void on_attach(message_bus_t& bus) override
        {
            auto self = subscriber_proxy_t{ id() };
            bus.subscribe(self.on_target<key_event_t>(
                [this](message_bus_t::context_t& context, const key_event_t& event)
                { context.publish_to(m_child.id(), event); }));
            bus.subscribe(self.on_target<resize_event_t>(
                [this](message_bus_t::context_t& context, const resize_event_t& event)
                { context.publish_to(m_child.id(), event); }));
            bus.subscribe(self.on_target<mouse_event_t>(
                [this](message_bus_t::context_t& context, const mouse_event_t& event)
                {
                    if (!m_has_last_inner)
                    {
                        return;
                    }

                    const auto lower = m_last_inner_lower;
                    const auto upper = m_last_inner_lower + m_last_inner_extent - location_t::ones();
                    const auto& pos = event.location;
                    const bool inside = lower[0] <= pos[0] && pos[0] <= upper[0] && lower[1] <= pos[1] && pos[1] <= upper[1];

                    if (inside)
                    {
                        mouse_event_t child_event = event;
                        child_event.location -= m_last_inner_lower;
                        context.publish_to(m_child.id(), child_event);
                    }
                }));

            m_child.m_impl->on_attach(bus);
        }

        void on_detach(message_bus_t& bus) override
        {
            bus.unsubscribe_subscriber(id());
            m_child.m_impl->on_detach(bus);
        }

    private:
        widget_t m_child;
        config_t m_cfg;
        mutable bool m_has_last_inner = false;
        mutable location_t m_last_inner_lower = {};
        mutable extent_t m_last_inner_extent = {};
    };

    inline widget_t operator()(widget_t child, config_t cfg) const
    {
        return widget_t::make<model_t>(std::move(child), std::move(cfg));
    }

    inline widget_t operator()(widget_t child) const { return (*this)(std::move(child), config_t{}); }
};

constexpr auto border = border_fn{};

}  // namespace widgets
}  // namespace ansi
}  // namespace zx

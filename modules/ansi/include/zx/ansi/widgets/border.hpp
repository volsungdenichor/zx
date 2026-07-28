#pragma once

#include <utility>
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
                return;
            }

            draw_border(view, view.bounds(), m_cfg.box_style, m_cfg.style);

            const auto inner_lower = mat::lower(view.bounds()) + location_t::ones();

            const extent_t inner_extent = size - extent_t::ones() * 2;
            const auto inner_bounds = mat::box::from_lower_extent(inner_lower, inner_extent);
            m_child.render(view.slice(mat::to_slice(inner_bounds)));
        }

        void on_attach() override { m_child.m_impl->on_attach(); }

        void on_detach() override { m_child.m_impl->on_detach(); }

    private:
        widget_t m_child;
        config_t m_cfg;
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

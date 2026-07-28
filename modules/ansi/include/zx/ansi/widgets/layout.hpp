#pragma once

#include <zx/event.hpp>
#include <zx/message_bus.hpp>
#include <zx/widget.hpp>

namespace zx
{
namespace ansi
{
namespace widgets
{
struct flex_t
{
    int fixed = 0;
    int flex = 1;

    static flex_t fixed_size(int n) { return { n, 0 }; }
    static flex_t flexible(int w = 1) { return { 0, w }; }
};

struct layout_child_t
{
    widget_t child;
    flex_t flex;

    layout_child_t(widget_t child_, flex_t flex_ = flex_t::flexible()) : child(std::move(child_)), flex(flex_) { }
};

template <std::size_t D>
inline std::vector<extent_t> compute_sizes(const std::vector<layout_child_t>& children, const extent_t& total)
{
    extent_t init = {};
    init[D] = 0;
    init[1 - D] = total[1 - D];
    std::vector<extent_t> sizes(children.size(), init);
    std::ptrdiff_t total_flex = 0;
    std::ptrdiff_t fixed_total = 0;

    for (std::size_t i = 0; i < children.size(); ++i)
    {
        const flex_t& f = children[i].flex;
        if (f.flex == 0)
        {
            sizes[i][D] = f.fixed > 0 ? f.fixed : children[i].child.preferred_size()[D];
            fixed_total += sizes[i][D];
        }
        else
        {
            total_flex += f.flex;
        }
    }

    const std::ptrdiff_t remaining = std::max(std::ptrdiff_t{ 0 }, total[D] - fixed_total);
    std::ptrdiff_t assigned_flex = 0;
    std::vector<std::size_t> flex_indices = {};
    flex_indices.reserve(children.size());

    for (std::size_t i = 0; i < children.size(); ++i)
    {
        if (children[i].flex.flex > 0)
        {
            sizes[i][D] = (total_flex > 0) ? (remaining * children[i].flex.flex / total_flex) : 0;
            assigned_flex += sizes[i][D];
            flex_indices.push_back(i);
        }
    }

    std::ptrdiff_t leftover = std::max<std::ptrdiff_t>(0, remaining - assigned_flex);
    for (std::size_t k = 0; leftover > 0 && k < flex_indices.size(); ++k, --leftover)
    {
        ++sizes[flex_indices[k]][D];
    }

    return sizes;
}

template <std::size_t D>
struct stack_fn
{
    struct model : widget_t::interface_t
    {
        explicit model(std::vector<layout_child_t> children) : m_children(std::move(children)) { focus_next(); }

        extent_t preferred_size() const override
        {
            extent_t result = {};
            for (const auto& lc : m_children)
            {
                const extent_t ps = lc.child.preferred_size();
                result[D] += (lc.flex.flex == 0 && lc.flex.fixed > 0) ? lc.flex.fixed : ps[D];
                result[1 - D] = std::max(result[1 - D], ps[1 - D]);
            }
            return result;
        }

        bool is_focused() const override { return m_focused_child >= 0; }

        void set_focused(bool focused) override
        {
            if (focused)
            {
                focus_next();
            }
            else
            {
                unfocus_all();
            }
        }

        void render(surface_t::mut_view_type view) const override
        {
            const auto sizes = compute_sizes<D>(m_children, mat::size(view.bounds()));
            m_last_child_bounds.assign(m_children.size(), bounds_t{});
            auto loc = mat::lower(view.bounds());
            for (std::size_t i = 0; i < m_children.size(); ++i)
            {
                extent_t size = sizes[i];
                m_last_child_bounds[i] = mat::box::from_lower_extent(loc, size);
                m_children[i].child.render(view.slice(mat::to_slice(m_last_child_bounds[i])));
                loc[D] += size[D];
            }
        }

        void on_attach(message_bus_t& bus) override
        {
            auto self = subscriber_proxy_t{ id() };
            bus.subscribe(self.on_target<mouse_event_t>(
                [this](message_bus_t::context_t& context, const mouse_event_t& event)
                {
                    const int idx = child_at(event.location);
                    if (idx >= 0 && idx < static_cast<int>(m_children.size()))
                    {
                        const bool is_left_down
                            = event.kind == mouse_event_kind_t::down && event.button == mouse_button_t::left;
                        const bool is_left_up_or_legacy_release = event.kind == mouse_event_kind_t::up;

                        if (is_left_down || is_left_up_or_legacy_release)
                        {
                            focus_at(idx);
                        }

                        mouse_event_t child_event = event;
                        const auto child_lower = m_last_child_bounds[static_cast<std::size_t>(idx)].get(
                            { mat::side_t::first, mat::side_t::first });
                        child_event.location -= child_lower;

                        context.publish_to(m_children[static_cast<std::size_t>(idx)].child.id(), child_event);
                    }
                }));

            bus.subscribe(self.on_target<key_event_t>(
                [this](message_bus_t::context_t& context, const key_event_t& event)
                {
                    if (const key_t* special = event.special())
                    {
                        if (*special == key_t::tab)
                        {
                            focus_next();
                            return;
                        }
                        if (*special == key_t::backtab)
                        {
                            focus_prev();
                            return;
                        }
                    }

                    if (m_focused_child >= 0 && m_focused_child < static_cast<int>(m_children.size()))
                    {
                        context.publish_to(m_children[static_cast<std::size_t>(m_focused_child)].child.id(), event);
                    }
                }));

            bus.subscribe(self.on_target<resize_event_t>(
                [this](message_bus_t::context_t& context, const resize_event_t& event)
                {
                    for (const auto& lc : m_children)
                    {
                        context.publish_to(lc.child.id(), event);
                    }
                }));

            for (auto& lc : m_children)
            {
                lc.child.on_attach(bus);
            }
        }

        void on_detach(message_bus_t& bus) override
        {
            bus.unsubscribe_subscriber(id());
            for (auto& lc : m_children)
            {
                lc.child.on_detach(bus);
            }
        }

        std::vector<layout_child_t> m_children;
        mutable std::vector<bounds_t> m_last_child_bounds;
        int m_focused_child = -1;

        void unfocus_all()
        {
            for (auto& lc : m_children)
            {
                lc.child.m_impl->set_focused(false);
            }
            m_focused_child = -1;
        }

        void focus_at(int idx)
        {
            unfocus_all();
            if (idx >= 0 && idx < static_cast<int>(m_children.size()))
            {
                m_children[static_cast<std::size_t>(idx)].child.m_impl->set_focused(true);
                m_focused_child = idx;
            }
        }

        void focus_next()
        {
            const int n = static_cast<int>(m_children.size());
            if (n <= 0)
            {
                m_focused_child = -1;
                return;
            }

            const int start = (m_focused_child + 1) % n;
            focus_at(start);
        }

        void focus_prev()
        {
            const int n = static_cast<int>(m_children.size());
            if (n <= 0)
            {
                m_focused_child = -1;
                return;
            }

            const int start = (m_focused_child - 1 + n) % n;
            focus_at(start);
        }

        int child_at(const location_t& pos) const
        {
            for (std::size_t i = 0; i < m_last_child_bounds.size(); ++i)
            {
                const auto lower = m_last_child_bounds[i].get({ mat::side_t::first, mat::side_t::first });
                const auto upper = m_last_child_bounds[i].get({ mat::side_t::last, mat::side_t::last });
                const bool inside = lower[0] <= pos[0] && pos[0] <= upper[0] && lower[1] <= pos[1] && pos[1] <= upper[1];
                if (inside)
                {
                    return static_cast<int>(i);
                }
            }
            return -1;
        }
    };

    widget_t operator()(std::vector<layout_child_t> children) const { return widget_t::make<model>(std::move(children)); }

    template <class... Tail>
    widget_t operator()(layout_child_t head, Tail... tail) const
    {
        std::vector<layout_child_t> children;
        children.reserve(sizeof...(Tail) + 1);
        children.push_back(std::move(head));
        (children.push_back(std::move(tail)), ...);
        return (*this)(std::move(children));
    }
};  // namespace widgets

constexpr auto vstack = stack_fn<0>{};
constexpr auto hstack = stack_fn<1>{};

}  // namespace widgets
}  // namespace ansi
}  // namespace zx
#pragma once

#include <zx/widget.hpp>

namespace zx
{
namespace ansi
{
namespace widgets
{

inline surface_t::slice_type to_slice(const surface_t::bounds_type& bounds)
{
    surface_t::slice_type result = {};
    for (std::size_t d = 0; d < 2; ++d)
    {
        result[d] = { bounds[d][0], bounds[d][1] };
    }
    return result;
}

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
    for (std::size_t i = 0; i < children.size(); ++i)
    {
        if (children[i].flex.flex > 0)
        {
            sizes[i][D] = (total_flex > 0) ? (remaining * children[i].flex.flex / total_flex) : 0;
        }
    }
    return sizes;
}

template <std::size_t D>
struct stack_fn
{
    struct model : widget_t::interface
    {
        model(std::vector<layout_child_t> children) : m_children(std::move(children)) { }

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

        bool is_focused() const override { return false; }

        void set_focused(bool focused) override { }

        void render(surface_t::mut_view_type view) const override
        {
            const auto sizes = compute_sizes<D>(m_children, mat::size(view.bounds()));
            m_last_child_bounds.assign(m_children.size(), bounds_t{});
            auto loc = view.bounds().get(mat::side_t::lower);
            for (std::size_t i = 0; i < m_children.size(); ++i)
            {
                extent_t size = sizes[i];
                m_last_child_bounds[i] = bounds_t::from_lower_size(loc, size);
                m_children[i].child.render(view.slice(to_slice(m_last_child_bounds[i])));
                loc[D] += size[D];
            }
        }

        void on_attach() override { }

        void on_detach() override { }

        std::vector<layout_child_t> m_children;
        mutable std::vector<bounds_t> m_last_child_bounds;
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

constexpr auto vstack = stack_fn<1>{};
constexpr auto hstack = stack_fn<0>{};

}  // namespace widgets
}  // namespace ansi
}  // namespace zx
#pragma once

#include <memory>
#include <zx/sequence.hpp>
#include <zx/surface.hpp>

namespace zx
{
namespace ansi
{

struct widget_t
{
    using id_type = std::size_t;

    struct interface
    {
        virtual ~interface() = default;

        virtual surface_t::size_type preferred_size() const { return {}; }

        virtual bool is_focused() const { return false; }

        virtual void set_focused(bool) { }

        virtual void render(surface_t::mut_view_type) const { }

        virtual id_type id() const { return reinterpret_cast<id_type>(this); }

        std::weak_ptr<interface> m_parent;
        std::vector<std::shared_ptr<interface>> m_children;

        maybe_t<const interface&> parent() const
        {
            if (auto parent = m_parent.lock())
            {
                return *parent;
            }
            return none;
        }

        sequence_t<const interface&> children() const
        {
            return seq::view(m_children)
                .transform([](const std::shared_ptr<interface>& impl) -> interface& { return *impl; });
        }
    };

    std::shared_ptr<interface> m_impl;

    explicit widget_t(std::shared_ptr<interface> impl) : m_impl{ std::move(impl) } { }

    template <class T, class... Args>
    static widget_t make(Args&&... args)
    {
        return widget_t{ std::make_shared<T>(std::forward<Args>(args)...) };
    }

    void append_child(widget_t child)
    {
        if (child.m_impl->m_parent.lock())
        {
            throw std::runtime_error{ "Child widget already has a parent" };
        }
        child.m_impl->m_parent = m_impl;
        m_impl->m_children.push_back(child.m_impl);
    }

    template <class... Tail>
    void append_children(widget_t child, Tail... tail)
    {
        append_child(std::move(child));
        if constexpr (sizeof...(Tail) > 0)
        {
            append_children(std::forward<Tail>(tail)...);
        }
    }

    void remove_child(widget_t child)
    {
        auto it = std::find_if(
            m_impl->m_children.begin(),
            m_impl->m_children.end(),
            [&](const std::shared_ptr<interface>& impl) { return impl->id() == child.id(); });
        if (it != m_impl->m_children.end())
        {
            m_impl->m_children.erase(it);
            child.m_impl->m_parent.reset();
        }
    }

    maybe_t<widget_t> parent() const
    {
        if (auto parent = m_impl->m_parent.lock())
        {
            return widget_t{ std::move(parent) };
        }
        return none;
    }

    widget_t child(std::size_t index) const { return widget_t{ m_impl->m_children.at(index) }; }

    sequence_t<widget_t> children() const
    {
        return seq::view(m_impl->m_children)
            .transform([](const std::shared_ptr<interface>& impl) { return widget_t{ impl }; });
    }

    id_type id() const { return m_impl->id(); }

    void render(surface_t::mut_view_type view) const { m_impl->render(view); }

    surface_t::size_type preferred_size() const { return m_impl->preferred_size(); }
};

}  // namespace ansi
}  // namespace zx

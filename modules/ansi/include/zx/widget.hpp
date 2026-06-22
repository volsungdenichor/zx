#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <typeindex>
#include <zx/sequence.hpp>
#include <zx/surface.hpp>
#include <utility>
#include <vector>

namespace zx
{
namespace ansi
{

struct subscription_bus_t
{
    using subscription_id_type = std::size_t;
    using subscriber_id_type = std::size_t;

    struct control_t
    {
        virtual ~control_t() = default;

        virtual void unsubscribe_self() = 0;
        virtual void unsubscribe(subscription_id_type subscription_id) = 0;
        virtual void unsubscribe_subscriber(subscriber_id_type subscriber_id) = 0;
    };

    virtual ~subscription_bus_t() = default;

    virtual subscription_id_type subscribe_erased(
        std::optional<subscriber_id_type> subscriber_id,
        std::type_index type,
        std::function<void(control_t&, const void*)> handler) = 0;

    virtual void unsubscribe(subscription_id_type subscription_id) = 0;

    virtual void unsubscribe_subscriber(subscriber_id_type subscriber_id) = 0;

    template <class E>
    subscription_id_type subscribe(subscriber_id_type subscriber_id, std::function<void(const E&)> handler)
    {
        return subscribe_erased(
            subscriber_id,
            typeid(E),
            [handler = std::move(handler)](control_t&, const void* event) { handler(*static_cast<const E*>(event)); });
    }

    template <class E>
    subscription_id_type subscribe(std::function<void(const E&)> handler)
    {
        return subscribe_erased(
            {},
            typeid(E),
            [handler = std::move(handler)](control_t&, const void* event) { handler(*static_cast<const E*>(event)); });
    }

    template <class E>
    subscription_id_type subscribe(
        subscriber_id_type subscriber_id, std::function<void(control_t&, const E&)> handler)
    {
        return subscribe_erased(
            subscriber_id,
            typeid(E),
            [handler = std::move(handler)](control_t& control, const void* event)
            { handler(control, *static_cast<const E*>(event)); });
    }

    template <class E>
    subscription_id_type subscribe(std::function<void(control_t&, const E&)> handler)
    {
        return subscribe_erased(
            {},
            typeid(E),
            [handler = std::move(handler)](control_t& control, const void* event)
            { handler(control, *static_cast<const E*>(event)); });
    }
};

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

        virtual void on_attach(subscription_bus_t&) { }

        virtual void on_detach(subscription_bus_t&) { }

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

    maybe_t<widget_t> root() const
    {
        if (auto p = parent())
        {
            return p->root();
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

    void on_attach(subscription_bus_t& bus) { m_impl->on_attach(bus); }

    void on_detach(subscription_bus_t& bus) { m_impl->on_detach(bus); }

    surface_t::size_type preferred_size() const { return m_impl->preferred_size(); }
};

}  // namespace ansi
}  // namespace zx

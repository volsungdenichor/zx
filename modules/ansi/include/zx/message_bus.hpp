#pragma once

#include <algorithm>
#include <functional>
#include <map>
#include <optional>
#include <typeindex>
#include <vector>
#include <zx/widget.hpp>

namespace zx
{
namespace ansi
{

enum class event_phase_t
{
    capture,
    target,
    bubble,
};

struct message_bus_t : public subscription_bus_t
{
    using widget_id_type = widget_t::id_type;
    using global_owner_id_type = std::size_t;

    struct control_t;
    template <class E>
    struct on_builder_t;

    using erased_handler_t = std::function<void(subscription_bus_t::control_t&, const void*)>;
    using erased_global_handler_t = std::function<void(const void*)>;

    struct subscription_t
    {
        widget_id_type widget_id;
        std::optional<std::weak_ptr<widget_t::interface>> widget;
        std::optional<event_phase_t> phase;
        erased_handler_t handler;
    };

    struct global_subscription_t
    {
        std::optional<global_owner_id_type> owner_id;
        erased_global_handler_t handler;
    };

    struct control_t : public subscription_bus_t::control_t
    {
        message_bus_t& self;
        widget_t widget;
        widget_t target_widget;
        event_phase_t phase;
        bool propagation_stopped = false;

        control_t(message_bus_t& bus, widget_t current_widget, widget_t target, event_phase_t current_phase)
            : self(bus)
            , widget(std::move(current_widget))
            , target_widget(std::move(target))
            , phase(current_phase)
        {
        }

        void stop_propagation() { propagation_stopped = true; }

        void unsubscribe_self() override { self.unsubscribe(widget.id()); }

        void unsubscribe(const widget_t& other_widget) { self.unsubscribe(other_widget.id()); }

        void unsubscribe(widget_id_type other_widget_id) override { self.unsubscribe(other_widget_id); }

        void unsubscribe_subscriber(subscriber_id_type other_subscriber_id) override
        {
            self.unsubscribe_subscriber(other_subscriber_id);
        }

        template <class E>
        void publish(const widget_t& target, const E& event)
        {
            self.publish(target, event);
        }

        template <class E>
        void publish_to_self(const E& event)
        {
            self.publish(widget, event);
        }

        template <class E>
        void publish_to_target(const E& event)
        {
            self.publish(target_widget, event);
        }

        template <class E>
        void publish_to(const widget_t& target, const E& event)
        {
            self.publish(target, event);
        }

        template <class E>
        void subscribe(const widget_t& other_widget, std::function<void(control_t&, const E&)> handler)
        {
            self.subscribe<E>(other_widget, std::move(handler));
        }

        template <class E>
        void subscribe(const widget_t& other_widget, std::function<void(const E&)> handler)
        {
            self.subscribe<E>(other_widget, std::move(handler));
        }

        template <class E>
        void on_capture(const widget_t& other_widget, std::function<void(control_t&, const E&)> handler)
        {
            self.on_capture<E>(other_widget, std::move(handler));
        }

        template <class E>
        void on_capture(const widget_t& other_widget, std::function<void(const E&)> handler)
        {
            self.on_capture<E>(other_widget, std::move(handler));
        }

        template <class E>
        void on_target(const widget_t& other_widget, std::function<void(control_t&, const E&)> handler)
        {
            self.on_target<E>(other_widget, std::move(handler));
        }

        template <class E>
        void on_target(const widget_t& other_widget, std::function<void(const E&)> handler)
        {
            self.on_target<E>(other_widget, std::move(handler));
        }

        template <class E>
        void on_bubble(const widget_t& other_widget, std::function<void(control_t&, const E&)> handler)
        {
            self.on_bubble<E>(other_widget, std::move(handler));
        }

        template <class E>
        void on_bubble(const widget_t& other_widget, std::function<void(const E&)> handler)
        {
            self.on_bubble<E>(other_widget, std::move(handler));
        }

        void unsubscribe_global(global_owner_id_type owner_id) { self.unsubscribe_global(owner_id); }

        template <class E>
        on_builder_t<E> on()
        {
            return self.on<E>();
        }
    };

    template <class E>
    struct on_builder_t
    {
        message_bus_t& self;

        on_builder_t& capture(const widget_t& widget, std::function<void(control_t&, const E&)> handler)
        {
            self.on_capture<E>(widget, std::move(handler));
            return *this;
        }

        on_builder_t& capture(const widget_t& widget, std::function<void(const E&)> handler)
        {
            self.on_capture<E>(widget, std::move(handler));
            return *this;
        }

        on_builder_t& target(const widget_t& widget, std::function<void(control_t&, const E&)> handler)
        {
            self.on_target<E>(widget, std::move(handler));
            return *this;
        }

        on_builder_t& target(const widget_t& widget, std::function<void(const E&)> handler)
        {
            self.on_target<E>(widget, std::move(handler));
            return *this;
        }

        on_builder_t& bubble(const widget_t& widget, std::function<void(control_t&, const E&)> handler)
        {
            self.on_bubble<E>(widget, std::move(handler));
            return *this;
        }

        on_builder_t& bubble(const widget_t& widget, std::function<void(const E&)> handler)
        {
            self.on_bubble<E>(widget, std::move(handler));
            return *this;
        }

        on_builder_t& any(const widget_t& widget, std::function<void(control_t&, const E&)> handler)
        {
            self.subscribe<E>(widget, std::move(handler));
            return *this;
        }

        on_builder_t& any(const widget_t& widget, std::function<void(const E&)> handler)
        {
            self.subscribe<E>(widget, std::move(handler));
            return *this;
        }

        on_builder_t& global(std::function<void(const E&)> handler)
        {
            self.subscribe_global<E>(std::move(handler));
            return *this;
        }

        on_builder_t& global(global_owner_id_type owner_id, std::function<void(const E&)> handler)
        {
            self.subscribe_global<E>(owner_id, std::move(handler));
            return *this;
        }

        on_builder_t& broadcast(std::function<void(const E&)> handler)
        {
            self.subscribe_global<E>(std::move(handler));
            return *this;
        }

        on_builder_t& broadcast(global_owner_id_type owner_id, std::function<void(const E&)> handler)
        {
            self.subscribe_global<E>(owner_id, std::move(handler));
            return *this;
        }
    };

    template <class E>
    on_builder_t<E> on()
    {
        return on_builder_t<E>{ *this };
    }

    template <class E>
    void subscribe_global(std::function<void(const E&)> handler)
    {
        subscribe_global_erased(
            std::nullopt,
            typeid(E),
            [handler = std::move(handler)](const void* e) { handler(*static_cast<const E*>(e)); });
    }

    template <class E>
    void subscribe_global(global_owner_id_type owner_id, std::function<void(const E&)> handler)
    {
        subscribe_global_erased(
            owner_id,
            typeid(E),
            [handler = std::move(handler)](const void* e) { handler(*static_cast<const E*>(e)); });
    }

    template <class E>
    void broadcast(const E& event)
    {
        ++m_publish_depth;
        dispatch_global(event);
        --m_publish_depth;
        if (m_publish_depth == 0)
        {
            flush_deferred_subscriptions();
            flush_deferred_unsubscriptions();
        }
    }

    template <class E>
    void publish(const E& event)
    {
        broadcast(event);
    }

    template <class E>
    void subscribe(const widget_t& widget, std::function<void(control_t&, const E&)> handler)
    {
        subscribe_with_phase_erased(
            widget,
            typeid(E),
            std::nullopt,
            [handler = std::move(handler)](subscription_bus_t::control_t& control, const void* e)
            { handler(static_cast<control_t&>(control), *static_cast<const E*>(e)); });
    }

    template <class E>
    void subscribe(const widget_t& widget, std::function<void(const E&)> handler)
    {
        subscribe_with_phase_erased(
            widget,
            typeid(E),
            std::nullopt,
            [handler = std::move(handler)](subscription_bus_t::control_t& control, const void* e)
            {
                static_cast<void>(control);
                handler(*static_cast<const E*>(e));
            });
    }

    template <class E>
    void on_capture(const widget_t& widget, std::function<void(control_t&, const E&)> handler)
    {
        subscribe_with_phase_erased(
            widget,
            typeid(E),
            event_phase_t::capture,
            [handler = std::move(handler)](subscription_bus_t::control_t& control, const void* e)
            { handler(static_cast<control_t&>(control), *static_cast<const E*>(e)); });
    }

    template <class E>
    void on_capture(const widget_t& widget, std::function<void(const E&)> handler)
    {
        subscribe_with_phase_erased(
            widget,
            typeid(E),
            event_phase_t::capture,
            [handler = std::move(handler)](subscription_bus_t::control_t& control, const void* e)
            {
                static_cast<void>(control);
                handler(*static_cast<const E*>(e));
            });
    }

    template <class E>
    void on_target(const widget_t& widget, std::function<void(control_t&, const E&)> handler)
    {
        subscribe_with_phase_erased(
            widget,
            typeid(E),
            event_phase_t::target,
            [handler = std::move(handler)](subscription_bus_t::control_t& control, const void* e)
            { handler(static_cast<control_t&>(control), *static_cast<const E*>(e)); });
    }

    template <class E>
    void on_target(const widget_t& widget, std::function<void(const E&)> handler)
    {
        subscribe_with_phase_erased(
            widget,
            typeid(E),
            event_phase_t::target,
            [handler = std::move(handler)](subscription_bus_t::control_t& control, const void* e)
            {
                static_cast<void>(control);
                handler(*static_cast<const E*>(e));
            });
    }

    template <class E>
    void on_bubble(const widget_t& widget, std::function<void(control_t&, const E&)> handler)
    {
        subscribe_with_phase_erased(
            widget,
            typeid(E),
            event_phase_t::bubble,
            [handler = std::move(handler)](subscription_bus_t::control_t& control, const void* e)
            { handler(static_cast<control_t&>(control), *static_cast<const E*>(e)); });
    }

    template <class E>
    void on_bubble(const widget_t& widget, std::function<void(const E&)> handler)
    {
        subscribe_with_phase_erased(
            widget,
            typeid(E),
            event_phase_t::bubble,
            [handler = std::move(handler)](subscription_bus_t::control_t& control, const void* e)
            {
                static_cast<void>(control);
                handler(*static_cast<const E*>(e));
            });
    }

    template <class E>
    void publish(const widget_t& target, const E& event)
    {
        const std::vector<widget_t> route = route_to_root(target);
        if (route.empty())
        {
            return;
        }

        ++m_publish_depth;

        bool propagation_stopped = false;

        if (route.size() >= 2)
        {
            for (std::size_t i = 0; i + 1 < route.size(); ++i)
            {
                propagation_stopped = dispatch_to_widget(route[i], target, event_phase_t::capture, event);
                if (propagation_stopped)
                {
                    break;
                }
            }
        }

        // Target phase
        if (!propagation_stopped)
        {
            propagation_stopped = dispatch_to_widget(route.back(), target, event_phase_t::target, event);
        }

        // Bubble: parent(target) -> root
        if (!propagation_stopped && route.size() >= 2)
        {
            for (std::size_t i = route.size() - 1; i > 0; --i)
            {
                propagation_stopped = dispatch_to_widget(route[i - 1], target, event_phase_t::bubble, event);
                if (propagation_stopped)
                {
                    break;
                }
            }
        }

        --m_publish_depth;
        if (m_publish_depth == 0)
        {
            flush_deferred_subscriptions();
            flush_deferred_unsubscriptions();
        }
    }

    void unsubscribe(const widget_t& widget) { unsubscribe(widget.id()); }

    void unsubscribe(widget_id_type widget_id) override
    {
        if (m_publish_depth > 0)
        {
            if (!is_pending_unsubscribe(widget_id))
            {
                m_deferred_unsubscriptions.push_back(widget_id);
            }
            return;
        }

        erase_widget_id(widget_id);
    }

    void unsubscribe_subscriber(subscriber_id_type subscriber_id) override
    {
        unsubscribe(subscriber_id);
    }

    void unsubscribe_global(global_owner_id_type owner_id)
    {
        if (m_publish_depth > 0)
        {
            if (!is_pending_global_unsubscribe(owner_id))
            {
                m_deferred_global_unsubscriptions.push_back(owner_id);
            }
            return;
        }

        erase_global_owner_id(owner_id);
    }

private:
    static std::vector<widget_t> route_to_root(const widget_t& target)
    {
        std::vector<widget_t> result;
        result.push_back(target);

        auto maybe_parent = target.parent();
        while (maybe_parent)
        {
            result.push_back(*maybe_parent);
            maybe_parent = maybe_parent->parent();
        }

        std::reverse(result.begin(), result.end());
        return result;
    }

    template <class E>
    bool dispatch_to_widget(const widget_t& current_widget, const widget_t& target, event_phase_t phase, const E& event)
    {
        const auto [b, e] = m_subscriptions.equal_range(typeid(E));
        for (auto it = b; it != e; ++it)
        {
            for (const auto& entry : it->second)
            {
                if (entry.widget_id != current_widget.id() || is_pending_unsubscribe(entry.widget_id))
                {
                    continue;
                }

                if (entry.phase && *entry.phase != phase)
                {
                    continue;
                }

                if (entry.widget && entry.widget->expired())
                {
                    if (!is_pending_unsubscribe(entry.widget_id))
                    {
                        m_deferred_unsubscriptions.push_back(entry.widget_id);
                    }
                    continue;
                }

                control_t control{ *this, current_widget, target, phase };
                entry.handler(control, &event);
                if (control.propagation_stopped)
                {
                    return true;
                }
            }
        }
        return false;
    }

    bool is_pending_unsubscribe(widget_id_type widget_id) const
    {
        return std::find(m_deferred_unsubscriptions.begin(), m_deferred_unsubscriptions.end(), widget_id)
               != m_deferred_unsubscriptions.end();
    }

    bool is_pending_global_unsubscribe(global_owner_id_type owner_id) const
    {
        return std::find(m_deferred_global_unsubscriptions.begin(), m_deferred_global_unsubscriptions.end(), owner_id)
               != m_deferred_global_unsubscriptions.end();
    }

    template <class E>
    void dispatch_global(const E& event)
    {
        const auto [b, e] = m_global_subscriptions.equal_range(typeid(E));
        for (auto it = b; it != e; ++it)
        {
            for (const auto& entry : it->second)
            {
                entry.handler(&event);
            }
        }
    }

    void flush_deferred_unsubscriptions()
    {
        for (const auto widget_id : m_deferred_unsubscriptions)
        {
            erase_widget_id(widget_id);
        }
        m_deferred_unsubscriptions.clear();

        for (const auto owner_id : m_deferred_global_unsubscriptions)
        {
            erase_global_owner_id(owner_id);
        }
        m_deferred_global_unsubscriptions.clear();
    }

    void flush_deferred_subscriptions()
    {
        for (auto& add_subscription : m_deferred_subscriptions)
        {
            add_subscription();
        }
        m_deferred_subscriptions.clear();
    }

    void erase_widget_id(widget_id_type widget_id)
    {
        for (auto& [type, handlers] : m_subscriptions)
        {
            (void)type;
            auto it = std::remove_if(
                handlers.begin(),
                handlers.end(),
                [widget_id](const subscription_t& entry) { return entry.widget_id == widget_id; });
            if (it != handlers.end())
            {
                handlers.erase(it, handlers.end());
            }
        }
    }

    void subscribe_with_phase_erased(
        widget_t widget,
        std::type_index type,
        std::optional<event_phase_t> phase,
        erased_handler_t handler)
    {
        subscription_t sub{ widget.id(), widget.m_impl, phase, std::move(handler) };

        if (m_publish_depth > 0)
        {
            m_deferred_subscriptions.push_back([this, type, sub = std::move(sub)]() mutable
                                               { m_subscriptions[type].push_back(std::move(sub)); });
            return;
        }

        m_subscriptions[type].push_back(std::move(sub));
    }

    subscription_id_type subscribe_erased(
        std::optional<subscriber_id_type> subscriber_id,
        std::type_index type,
        std::function<void(subscription_bus_t::control_t&, const void*)> handler) override
    {
        const subscription_id_type subscription_id = ++m_last_subscription_id;
        subscription_t sub{ subscriber_id.value_or(subscription_id), std::nullopt, std::nullopt, std::move(handler) };

        if (m_publish_depth > 0)
        {
            m_deferred_subscriptions.push_back([this, type, sub = std::move(sub)]() mutable
                                               { m_subscriptions[type].push_back(std::move(sub)); });
            return subscription_id;
        }

        m_subscriptions[type].push_back(std::move(sub));
        return subscription_id;
    }

    void subscribe_global_erased(std::type_index type, erased_global_handler_t handler)
    {
        global_subscription_t sub{ std::nullopt, std::move(handler) };

        if (m_publish_depth > 0)
        {
            m_deferred_subscriptions.push_back(
                [this, type, sub = std::move(sub)]() mutable { m_global_subscriptions[type].push_back(std::move(sub)); });
            return;
        }

        m_global_subscriptions[type].push_back(std::move(sub));
    }

    void subscribe_global_erased(
        std::optional<global_owner_id_type> owner_id,
        std::type_index type,
        erased_global_handler_t handler)
    {
        global_subscription_t sub{ owner_id, std::move(handler) };

        if (m_publish_depth > 0)
        {
            m_deferred_subscriptions.push_back(
                [this, type, sub = std::move(sub)]() mutable { m_global_subscriptions[type].push_back(std::move(sub)); });
            return;
        }

        m_global_subscriptions[type].push_back(std::move(sub));
    }

    void erase_global_owner_id(global_owner_id_type owner_id)
    {
        for (auto& [type, handlers] : m_global_subscriptions)
        {
            (void)type;
            auto it = std::remove_if(
                handlers.begin(),
                handlers.end(),
                [owner_id](const global_subscription_t& entry)
                { return entry.owner_id && *entry.owner_id == owner_id; });
            if (it != handlers.end())
            {
                handlers.erase(it, handlers.end());
            }
        }
    }

private:
    std::size_t m_last_subscription_id = 0;
    std::size_t m_publish_depth = 0;
    std::vector<std::function<void()>> m_deferred_subscriptions;
    std::vector<widget_id_type> m_deferred_unsubscriptions;
    std::vector<global_owner_id_type> m_deferred_global_unsubscriptions;
    std::map<std::type_index, std::vector<subscription_t>> m_subscriptions;
    std::map<std::type_index, std::vector<global_subscription_t>> m_global_subscriptions;
};

}  // namespace ansi
}  // namespace zx

#pragma once

#include <algorithm>
#include <functional>
#include <map>
#include <optional>
#include <typeindex>
#include <vector>
#include <zx/maybe.hpp>
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
            std::nullopt, typeid(E), [handler = std::move(handler)](const void* e) { handler(*static_cast<const E*>(e)); });
    }

    template <class E>
    void subscribe_global(global_owner_id_type owner_id, std::function<void(const E&)> handler)
    {
        subscribe_global_erased(
            owner_id, typeid(E), [handler = std::move(handler)](const void* e) { handler(*static_cast<const E*>(e)); });
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

    void unsubscribe_subscriber(subscriber_id_type subscriber_id) override { unsubscribe(subscriber_id); }

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
        widget_t widget, std::type_index type, std::optional<event_phase_t> phase, erased_handler_t handler)
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
            m_deferred_subscriptions.push_back([this, type, sub = std::move(sub)]() mutable
                                               { m_global_subscriptions[type].push_back(std::move(sub)); });
            return;
        }

        m_global_subscriptions[type].push_back(std::move(sub));
    }

    void subscribe_global_erased(
        std::optional<global_owner_id_type> owner_id, std::type_index type, erased_global_handler_t handler)
    {
        global_subscription_t sub{ owner_id, std::move(handler) };

        if (m_publish_depth > 0)
        {
            m_deferred_subscriptions.push_back([this, type, sub = std::move(sub)]() mutable
                                               { m_global_subscriptions[type].push_back(std::move(sub)); });
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
                [owner_id](const global_subscription_t& entry) { return entry.owner_id && *entry.owner_id == owner_id; });
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

namespace v2
{

enum class event_phase_t
{
    capture,
    target,
    bubble,
};

struct message_bus_t
{
    using subscription_id_type = std::size_t;
    using subscriber_id_type = std::size_t;

    struct context_t;

    using erased_handler_t = std::function<void(context_t&, const void*)>;

    using route_builder_t = std::function<maybe_t<subscriber_id_type>(subscriber_id_type)>;

    struct default_route_builder_t
    {
        maybe_t<subscriber_id_type> operator()(subscriber_id_type) const { return {}; }
    };

    template <class T>
    struct handler_impl_t
    {
        std::function<void(context_t&, const T&)> handler;

        void operator()(context_t& context, const void* event) const { handler(context, *static_cast<const T*>(event)); }
    };

    template <class T>
    struct short_handler_impl_t
    {
        std::function<void(const T&)> handler;

        void operator()(context_t&, const void* event) const { handler(*static_cast<const T*>(event)); }
    };

    struct subscription_info_t
    {
        std::type_index type;
        maybe_t<subscriber_id_type> subscriber_id;
        maybe_t<event_phase_t> phase;
        erased_handler_t handler;
    };

    struct subscription_t
    {
        subscription_id_type subscription_id;
        maybe_t<subscriber_id_type> subscriber_id;
        maybe_t<event_phase_t> phase;
        erased_handler_t handler;
    };

    explicit message_bus_t(route_builder_t route_builder = default_route_builder_t{})
        : m_route_builder(std::move(route_builder))
    {
    }

    void set_route_builder(route_builder_t route_builder) { m_route_builder = std::move(route_builder); }

    void step_in() { ++m_publish_depth; }
    void step_out()
    {
        if (m_publish_depth == 0)
        {
            throw std::runtime_error("message_bus_t: step_out() called without matching step_in()");
        }
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
        step_in();
        dispatch(std::type_index(typeid(E)), &event);
        step_out();
    }

    template <class E>
    void publish_to(subscriber_id_type subscriber_id, const E& event)
    {
        const auto type = std::type_index(typeid(E));
        const void* e = &event;
        const std::vector<subscriber_id_type> route = route_to_root(subscriber_id);
        if (route.empty())
        {
            return;
        }

        step_in();

        bool propagation_stopped = false;

        if (route.size() >= 2)
        {
            for (std::size_t i = 0; i + 1 < route.size(); ++i)
            {
                propagation_stopped = dispatch(type, route[i], subscriber_id, event_phase_t::capture, e);
                if (propagation_stopped)
                {
                    break;
                }
            }
        }

        if (!propagation_stopped)
        {
            propagation_stopped = dispatch(type, route.back(), subscriber_id, event_phase_t::target, e);
        }

        if (!propagation_stopped && route.size() >= 2)
        {
            for (std::size_t i = route.size() - 1; i > 0; --i)
            {
                propagation_stopped = dispatch(type, route[i - 1], subscriber_id, event_phase_t::bubble, e);
                if (propagation_stopped)
                {
                    break;
                }
            }
        }

        step_out();
    }

    subscription_id_type subscribe(subscription_info_t info)
    {
        return do_subscribe(info.type, std::move(info.subscriber_id), std::move(info.phase), std::move(info.handler));
    }

    void unsubscribe(subscription_id_type id)
    {
        if (m_publish_depth > 0)
        {
            if (!is_pending_unsubscription(id))
            {
                m_deferred_unsubscriptions.insert(id);
            }
            return;
        }

        do_unsubscribe(id);
    }

    void unsubscribe_subscriber(subscriber_id_type subscriber_id)
    {
        if (m_publish_depth > 0)
        {
            for (const auto& [type, subscriptions] : m_subscriptions)
            {
                (void)type;
                for (const auto& entry : subscriptions)
                {
                    if (entry.subscriber_id && *entry.subscriber_id == subscriber_id
                        && !is_pending_unsubscription(entry.subscription_id))
                    {
                        m_deferred_unsubscriptions.insert(entry.subscription_id);
                    }
                }
            }

            for (const auto& [type, entry] : m_deferred_subscriptions)
            {
                (void)type;
                if (entry.subscriber_id && *entry.subscriber_id == subscriber_id
                    && !is_pending_unsubscription(entry.subscription_id))
                {
                    m_deferred_unsubscriptions.insert(entry.subscription_id);
                }
            }
            return;
        }

        do_unsubscribe_subscriber(subscriber_id);
    }

    subscription_id_type do_subscribe(
        std::type_index type,
        maybe_t<subscriber_id_type> subscriber_id,
        maybe_t<event_phase_t> phase,
        erased_handler_t handler)
    {
        subscription_id_type subscription_id = m_id_manager.next_subscription_id();
        subscription_t sub{ subscription_id, subscriber_id, phase, std::move(handler) };

        if (m_publish_depth > 0)
        {
            return defer_subscription(type, std::move(sub));
        }

        m_subscriptions[type].push_back(std::move(sub));
        return subscription_id;
    }

    subscription_id_type defer_subscription(std::type_index type, subscription_t sub)
    {
        subscription_id_type id = sub.subscription_id;
        m_deferred_subscriptions.push_back({ type, std::move(sub) });
        return id;
    }

    struct context_t
    {
        struct subscriber_ids_t
        {
            maybe_t<subscriber_id_type> handler_id;
            maybe_t<subscriber_id_type> current_id;
            maybe_t<subscriber_id_type> target_id;
        };

        message_bus_t& m_self;
        subscription_id_type id;
        subscriber_ids_t subscriber_ids;
        maybe_t<event_phase_t> phase;
        bool m_propagation_stopped = false;

        void stop_propagation() { m_propagation_stopped = true; }

        template <class E>
        void publish(const E& event)
        {
            m_self.publish(event);
        }

        template <class E>
        void publish_to(subscriber_id_type target_id, const E& event)
        {
            m_self.publish_to(target_id, event);
        }

        template <class E>
        void publish_to_self(const E& event)
        {
            if (subscriber_ids.handler_id)
            {
                m_self.publish_to(*subscriber_ids.handler_id, event);
            }
        }

        void unsubscribe_self() { m_self.unsubscribe(id); }

        void unsubscribe(subscription_id_type other_id) { m_self.unsubscribe(other_id); }

        void unsubscribe_subscriber(subscriber_id_type other_subscriber_id)
        {
            m_self.unsubscribe_subscriber(other_subscriber_id);
        }

        subscription_id_type subscribe(subscription_info_t info) { return m_self.subscribe(std::move(info)); }
    };

    bool is_pending_unsubscription(subscription_id_type id) { return m_deferred_unsubscriptions.count(id) > 0; }

    void flush_deferred_unsubscriptions()
    {
        for (subscription_id_type id : m_deferred_unsubscriptions)
        {
            do_unsubscribe(id);
        }
        m_deferred_unsubscriptions.clear();
    }

    void flush_deferred_subscriptions()
    {
        for (auto& [type, sub] : m_deferred_subscriptions)
        {
            m_subscriptions[type].push_back(std::move(sub));
        }
        m_deferred_subscriptions.clear();
    }

    void do_unsubscribe(subscription_id_type id)
    {
        for (auto& [type, subscriptions] : m_subscriptions)
        {
            (void)type;
            subscriptions.erase(
                std::remove_if(
                    subscriptions.begin(),
                    subscriptions.end(),
                    [&](const subscription_t& entry) { return entry.subscription_id == id; }),
                subscriptions.end());
        }
    }

    void dispatch(std::type_index type, const void* event)
    {
        const auto [b, e] = m_subscriptions.equal_range(type);
        for (auto it = b; it != e; ++it)
        {
            for (const auto& entry : it->second)
            {
                if (is_pending_unsubscription(entry.subscription_id))
                {
                    continue;
                }

                context_t context{ *this, entry.subscription_id, typename context_t::subscriber_ids_t{ none, none, none }, none };
                entry.handler(context, event);
            }
        }
    }

    bool dispatch(
        std::type_index type,
        subscriber_id_type current,
        [[maybe_unused]] subscriber_id_type target,
        event_phase_t phase,
        const void* event)
    {
        const auto [b, e] = m_subscriptions.equal_range(type);
        for (auto it = b; it != e; ++it)
        {
            for (const auto& entry : it->second)
            {
                if (is_pending_unsubscription(entry.subscription_id))
                {
                    continue;
                }

                if (entry.subscriber_id && *entry.subscriber_id != current)
                {
                    continue;
                }

                if (entry.phase && *entry.phase != phase)
                {
                    continue;
                }

                context_t context{
                    *this,
                    entry.subscription_id,
                    typename context_t::subscriber_ids_t{ entry.subscriber_id, current, target },
                    phase
                };
                entry.handler(context, event);
                if (context.m_propagation_stopped)
                {
                    return true;
                }
            }
        }
        return false;
    }

    struct id_manager_t
    {
        subscription_id_type m_last_subscription_id = 0;
        subscription_id_type next_subscription_id() { return ++m_last_subscription_id; }
    };

    std::vector<subscriber_id_type> route_to_root(subscriber_id_type target) const
    {
        std::vector<subscriber_id_type> route = { target };
        if (m_route_builder)
        {
            while (true)
            {
                if (auto maybe_parent = m_route_builder(route.back()); maybe_parent)
                {
                    route.push_back(*maybe_parent);
                }
                else
                {
                    break;
                }
            }
        }
        std::reverse(route.begin(), route.end());
        return route;
    }

    void do_unsubscribe_subscriber(subscriber_id_type subscriber_id)
    {
        for (auto& [type, subscriptions] : m_subscriptions)
        {
            (void)type;
            subscriptions.erase(
                std::remove_if(
                    subscriptions.begin(),
                    subscriptions.end(),
                    [&](const subscription_t& entry)
                    { return entry.subscriber_id && *entry.subscriber_id == subscriber_id; }),
                subscriptions.end());
        }

        m_deferred_subscriptions.erase(
            std::remove_if(
                m_deferred_subscriptions.begin(),
                m_deferred_subscriptions.end(),
                [&](const std::pair<std::type_index, subscription_t>& deferred)
                {
                    const auto& entry = deferred.second;
                    return entry.subscriber_id && *entry.subscriber_id == subscriber_id;
                }),
            m_deferred_subscriptions.end());
    }

    std::map<std::type_index, std::vector<subscription_t>> m_subscriptions = {};
    std::vector<std::pair<std::type_index, subscription_t>> m_deferred_subscriptions = {};
    std::set<subscription_id_type> m_deferred_unsubscriptions;
    int m_publish_depth = 0;
    id_manager_t m_id_manager = {};
    route_builder_t m_route_builder;
};

struct subscriber_proxy_t
{
    message_bus_t::subscriber_id_type m_subscriber_id;

    template <class E>
    message_bus_t::subscription_info_t on(std::function<void(message_bus_t::context_t&, const E&)> handler) const
    {
        return message_bus_t::subscription_info_t{
            typeid(E), m_subscriber_id, none, message_bus_t::handler_impl_t<E>{ std::move(handler) }
        };
    }

    template <class E>
    message_bus_t::subscription_info_t on(std::function<void(const E&)> handler) const
    {
        return message_bus_t::subscription_info_t{
            typeid(E), m_subscriber_id, none, message_bus_t::short_handler_impl_t<E>{ std::move(handler) }
        };
    }

    template <class E>
    message_bus_t::subscription_info_t on_capture(std::function<void(message_bus_t::context_t&, const E&)> handler) const
    {
        return message_bus_t::subscription_info_t{
            typeid(E), m_subscriber_id, event_phase_t::capture, message_bus_t::handler_impl_t<E>{ std::move(handler) }
        };
    }

    template <class E>
    message_bus_t::subscription_info_t on_capture(std::function<void(const E&)> handler) const
    {
        return message_bus_t::subscription_info_t{
            typeid(E), m_subscriber_id, event_phase_t::capture, message_bus_t::short_handler_impl_t<E>{ std::move(handler) }
        };
    }

    template <class E>
    message_bus_t::subscription_info_t on_target(std::function<void(message_bus_t::context_t&, const E&)> handler) const
    {
        return message_bus_t::subscription_info_t{
            typeid(E), m_subscriber_id, event_phase_t::target, message_bus_t::handler_impl_t<E>{ std::move(handler) }
        };
    }

    template <class E>
    message_bus_t::subscription_info_t on_target(std::function<void(const E&)> handler) const
    {
        return message_bus_t::subscription_info_t{
            typeid(E), m_subscriber_id, event_phase_t::target, message_bus_t::short_handler_impl_t<E>{ std::move(handler) }
        };
    }

    template <class E>
    message_bus_t::subscription_info_t on_bubble(std::function<void(message_bus_t::context_t&, const E&)> handler) const
    {
        return message_bus_t::subscription_info_t{
            typeid(E), m_subscriber_id, event_phase_t::bubble, message_bus_t::handler_impl_t<E>{ std::move(handler) }
        };
    }

    template <class E>
    message_bus_t::subscription_info_t on_bubble(std::function<void(const E&)> handler) const
    {
        return message_bus_t::subscription_info_t{
            typeid(E), m_subscriber_id, event_phase_t::bubble, message_bus_t::short_handler_impl_t<E>{ std::move(handler) }
        };
    }
};

template <class E>
message_bus_t::subscription_info_t on(std::function<void(message_bus_t::context_t&, const E&)> handler)
{
    return message_bus_t::subscription_info_t{
        typeid(E), none, none, message_bus_t::handler_impl_t<E>{ std::move(handler) }
    };
}

template <class E>
message_bus_t::subscription_info_t on(std::function<void(const E&)> handler)
{
    return message_bus_t::subscription_info_t{
        typeid(E), none, none, message_bus_t::short_handler_impl_t<E>{ std::move(handler) }
    };
}

}  // namespace v2

}  // namespace ansi
}  // namespace zx

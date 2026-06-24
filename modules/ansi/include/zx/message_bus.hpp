#pragma once

#include <algorithm>
#include <functional>
#include <map>
#include <set>
#include <stdexcept>
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

struct message_bus_t
{
    using subscription_id_type = std::size_t;
    using subscriber_id_type = std::size_t;

    struct context_t;

    using erased_handler_t = std::function<void(context_t&, const void*)>;

    struct publish_scope_guard_t
    {
        message_bus_t& self;

        explicit publish_scope_guard_t(message_bus_t& bus) : self(bus) { self.step_in(); }

        ~publish_scope_guard_t() { self.step_out(); }
    };

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

    void set_route_builder(route_builder_t route_builder)
    {
        m_route_builder = std::move(route_builder);
        invalidate_all_routes();
    }

    void invalidate_route(subscriber_id_type subscriber_id) { m_route_cache.erase(subscriber_id); }

    void invalidate_all_routes() { m_route_cache.clear(); }

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
        publish_scope_guard_t publish_scope{ *this };
        dispatch(std::type_index(typeid(E)), &event);
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

        publish_scope_guard_t publish_scope{ *this };

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

                context_t context{
                    *this, entry.subscription_id, typename context_t::subscriber_ids_t{ none, none, none }, none
                };
                entry.handler(context, event);
            }
        }
    }

    bool dispatch(
        std::type_index type, subscriber_id_type current, subscriber_id_type target, event_phase_t phase, const void* event)
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

                context_t context{ *this,
                                   entry.subscription_id,
                                   typename context_t::subscriber_ids_t{ entry.subscriber_id, current, target },
                                   phase };
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
        if (const auto it = m_route_cache.find(target); it != m_route_cache.end())
        {
            return it->second;
        }

        std::vector<subscriber_id_type> route = { target };
        constexpr std::size_t max_route_depth = 1024;
        if (m_route_builder)
        {
            while (route.size() < max_route_depth)
            {
                if (auto maybe_parent = m_route_builder(route.back()); maybe_parent)
                {
                    if (std::find(route.begin(), route.end(), *maybe_parent) != route.end())
                    {
                        throw std::runtime_error("message_bus_t: cycle detected while building subscriber route");
                    }
                    route.push_back(*maybe_parent);
                }
                else
                {
                    break;
                }
            }

            if (route.size() == max_route_depth)
            {
                throw std::runtime_error("message_bus_t: subscriber route exceeded maximum depth");
            }
        }
        std::reverse(route.begin(), route.end());
        m_route_cache[target] = route;
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
    mutable std::map<subscriber_id_type, std::vector<subscriber_id_type>> m_route_cache;
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

}  // namespace ansi
}  // namespace zx

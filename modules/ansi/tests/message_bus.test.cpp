#include <gmock/gmock.h>

#include <zx/format.hpp>
#include <zx/message_bus.hpp>

namespace
{

struct test_event_t
{
    int value = 0;
};

struct test_widget_t : zx::ansi::widget_t::interface
{
};

std::string_view phase_name(zx::ansi::event_phase_t phase)
{
    switch (phase)
    {
        case zx::ansi::event_phase_t::capture: return "capture";
        case zx::ansi::event_phase_t::target: return "target";
        case zx::ansi::event_phase_t::bubble: return "bubble";
    }
    return "unknown";
}

}  // namespace

TEST(message_bus, dispatches_capture_target_and_bubble_in_tree_order)
{
    zx::ansi::message_bus_t bus;

    auto root = zx::ansi::widget_t::make<test_widget_t>();
    auto parent = zx::ansi::widget_t::make<test_widget_t>();
    auto target = zx::ansi::widget_t::make<test_widget_t>();
    root.append_child(parent);
    parent.append_child(target);

    std::vector<std::string> calls;

    bus.on<test_event_t>()
        .capture(
            root,
            [&](zx::ansi::message_bus_t::control_t& control, const test_event_t&)
            { calls.push_back(zx::format(phase_name(control.phase), ":root")); })
        .capture(
            parent,
            [&](zx::ansi::message_bus_t::control_t& control, const test_event_t&)
            { calls.push_back(zx::format(phase_name(control.phase), ":parent")); })
        .target(
            target,
            [&](zx::ansi::message_bus_t::control_t& control, const test_event_t&)
            { calls.push_back(zx::format(phase_name(control.phase), ":target")); })
        .bubble(
            parent,
            [&](zx::ansi::message_bus_t::control_t& control, const test_event_t&)
            { calls.push_back(zx::format(phase_name(control.phase), ":parent")); })
        .bubble(
            root,
            [&](zx::ansi::message_bus_t::control_t& control, const test_event_t&)
            { calls.push_back(zx::format(phase_name(control.phase), ":root")); });

    bus.publish(target, test_event_t{});

    EXPECT_THAT(
        calls, testing::ElementsAre("capture:root", "capture:parent", "target:target", "bubble:parent", "bubble:root"));
}

TEST(message_bus, unsubscribe_uses_widget_id_and_removes_widget_handlers)
{
    zx::ansi::message_bus_t bus;

    auto root = zx::ansi::widget_t::make<test_widget_t>();
    auto target = zx::ansi::widget_t::make<test_widget_t>();
    root.append_child(target);

    int calls = 0;
    bus.on_target<test_event_t>(target, [&](const test_event_t&) { ++calls; });

    bus.publish(target, test_event_t{});
    bus.unsubscribe(target.id());
    bus.publish(target, test_event_t{});

    EXPECT_THAT(calls, testing::Eq(1));
}

TEST(message_bus, stop_propagation_stops_remaining_phases)
{
    zx::ansi::message_bus_t bus;

    auto root = zx::ansi::widget_t::make<test_widget_t>();
    auto parent = zx::ansi::widget_t::make<test_widget_t>();
    auto target = zx::ansi::widget_t::make<test_widget_t>();
    root.append_child(parent);
    parent.append_child(target);

    std::vector<std::string> calls;

    bus.on_capture<test_event_t>(
        root,
        [&](zx::ansi::message_bus_t::control_t& control, const test_event_t&)
        { calls.push_back(zx::format(phase_name(control.phase), ":root")); });

    bus.on_capture<test_event_t>(
        parent,
        [&](zx::ansi::message_bus_t::control_t& control, const test_event_t&)
        {
            calls.push_back(zx::format(phase_name(control.phase), ":parent"));
            if (control.phase == zx::ansi::event_phase_t::capture)
            {
                control.stop_propagation();
            }
        });

    bus.on_target<test_event_t>(
        target,
        [&](zx::ansi::message_bus_t::control_t& control, const test_event_t&)
        { calls.push_back(zx::format(phase_name(control.phase), ":target")); });

    bus.publish(target, test_event_t{});

    EXPECT_THAT(calls, testing::ElementsAre("capture:root", "capture:parent"));
}

TEST(message_bus, global_subscription_receives_broadcast_without_widget_target)
{
    zx::ansi::message_bus_t bus;

    std::vector<int> values;
    bus.subscribe_global<test_event_t>([&](const test_event_t& event) { values.push_back(event.value); });

    bus.publish(test_event_t{ 7 });
    bus.broadcast(test_event_t{ 9 });

    EXPECT_THAT(values, testing::ElementsAre(7, 9));
}

TEST(message_bus, fluent_builder_supports_global_broadcast_handlers)
{
    zx::ansi::message_bus_t bus;

    int value_sum = 0;
    bus.on<test_event_t>().global([&](const test_event_t& event) { value_sum += event.value; });

    bus.publish(test_event_t{ 3 });
    bus.publish(test_event_t{ 4 });

    EXPECT_THAT(value_sum, testing::Eq(7));
}

TEST(message_bus, owner_scoped_global_unsubscribe_removes_only_matching_owner)
{
    zx::ansi::message_bus_t bus;

    constexpr zx::ansi::message_bus_t::global_owner_id_type owner_a = 100;
    constexpr zx::ansi::message_bus_t::global_owner_id_type owner_b = 200;

    int sum_a = 0;
    int sum_b = 0;
    int sum_anonymous = 0;

    bus.subscribe_global<test_event_t>(owner_a, [&](const test_event_t& e) { sum_a += e.value; });
    bus.subscribe_global<test_event_t>(owner_b, [&](const test_event_t& e) { sum_b += e.value; });
    bus.subscribe_global<test_event_t>([&](const test_event_t& e) { sum_anonymous += e.value; });

    bus.publish(test_event_t{ 1 });
    bus.unsubscribe_global(owner_a);
    bus.publish(test_event_t{ 2 });

    EXPECT_THAT(sum_a, testing::Eq(1));
    EXPECT_THAT(sum_b, testing::Eq(3));
    EXPECT_THAT(sum_anonymous, testing::Eq(3));
}

TEST(message_bus, fluent_builder_supports_owner_scoped_global_unsubscribe)
{
    zx::ansi::message_bus_t bus;

    constexpr zx::ansi::message_bus_t::global_owner_id_type owner = 42;

    int sum = 0;
    bus.on<test_event_t>().global(owner, [&](const test_event_t& e) { sum += e.value; });

    bus.publish(test_event_t{ 5 });
    bus.unsubscribe_global(owner);
    bus.publish(test_event_t{ 7 });

    EXPECT_THAT(sum, testing::Eq(5));
}

TEST(message_bus, publish_recovers_after_handler_throws)
{
    zx::ansi::message_bus_t bus;

    auto target = zx::ansi::widget_t::make<test_widget_t>();

    bool should_throw = true;
    int calls = 0;

    bus.on_target<test_event_t>(
        target,
        [&](zx::ansi::message_bus_t::control_t&, const test_event_t&)
        {
            if (should_throw)
            {
                should_throw = false;
                throw std::runtime_error("boom");
            }
            ++calls;
        });

    try
    {
        bus.publish(target, test_event_t{});
    }
    catch (const std::runtime_error&)
    {
    }

    bus.on_target<test_event_t>(target, [&](const test_event_t&) { calls += 10; });
    bus.publish(target, test_event_t{});

    EXPECT_THAT(calls, testing::Eq(11));
}

TEST(message_bus_v2, publish_to_dispatches_capture_target_and_bubble_in_route_order)
{
    using namespace zx::ansi::v2;
    message_bus_t bus{ [](message_bus_t::subscriber_id_type target) -> zx::maybe_t<message_bus_t::subscriber_id_type>
                       {
                           switch (target)
                           {
                               case 3: return 2;
                               case 2: return 1;
                               case 1: return zx::none;
                               default: return zx::none;
                           }
                       } };

    std::vector<std::string> calls;

    bus.subscribe(subscriber_proxy_t{ 1 }.on_capture<test_event_t>([&](message_bus_t::context_t&, const test_event_t&)
                                                                   { calls.push_back("capture:1"); }));
    bus.subscribe(subscriber_proxy_t{ 2 }.on_capture<test_event_t>([&](message_bus_t::context_t&, const test_event_t&)
                                                                   { calls.push_back("capture:2"); }));
    bus.subscribe(subscriber_proxy_t{ 3 }.on_target<test_event_t>([&](message_bus_t::context_t&, const test_event_t&)
                                                                  { calls.push_back("target:3"); }));
    bus.subscribe(subscriber_proxy_t{ 2 }.on_bubble<test_event_t>([&](message_bus_t::context_t&, const test_event_t&)
                                                                  { calls.push_back("bubble:2"); }));
    bus.subscribe(subscriber_proxy_t{ 1 }.on_bubble<test_event_t>([&](message_bus_t::context_t&, const test_event_t&)
                                                                  { calls.push_back("bubble:1"); }));

    bus.publish_to(3, test_event_t{});

    EXPECT_THAT(calls, testing::ElementsAre("capture:1", "capture:2", "target:3", "bubble:2", "bubble:1"));
}

TEST(message_bus_v2, publish_to_throws_on_cyclic_route_builder)
{
    using namespace zx::ansi::v2;
    message_bus_t bus{ [](message_bus_t::subscriber_id_type target) -> zx::maybe_t<message_bus_t::subscriber_id_type>
                       {
                           switch (target)
                           {
                               case 1: return 2;
                               case 2: return 1;
                               default: return zx::none;
                           }
                       } };

    EXPECT_THROW(bus.publish_to(1, test_event_t{}), std::runtime_error);
}

TEST(message_bus_v2, publish_to_caches_route_per_target)
{
    using namespace zx::ansi::v2;

    int route_builder_calls = 0;
    message_bus_t bus{ [&](message_bus_t::subscriber_id_type target) -> zx::maybe_t<message_bus_t::subscriber_id_type>
                       {
                           ++route_builder_calls;
                           switch (target)
                           {
                               case 3: return 2;
                               case 2: return 1;
                               case 1: return zx::none;
                               default: return zx::none;
                           }
                       } };

    bus.subscribe(zx::ansi::v2::subscriber_proxy_t{ 3 }.on_target<test_event_t>([](const test_event_t&) {}));

    bus.publish_to(3, test_event_t{});
    bus.publish_to(3, test_event_t{});

    EXPECT_THAT(route_builder_calls, testing::Eq(3));
}

TEST(message_bus_v2, invalidate_route_and_invalidate_all_routes_force_rebuild)
{
    using namespace zx::ansi::v2;

    int route_builder_calls = 0;
    message_bus_t bus{ [&](message_bus_t::subscriber_id_type target) -> zx::maybe_t<message_bus_t::subscriber_id_type>
                       {
                           ++route_builder_calls;
                           switch (target)
                           {
                               case 3: return 2;
                               case 2: return 1;
                               case 1: return zx::none;
                               case 5: return 4;
                               case 4: return zx::none;
                               default: return zx::none;
                           }
                       } };

    bus.subscribe(zx::ansi::v2::subscriber_proxy_t{ 3 }.on_target<test_event_t>([](const test_event_t&) {}));
    bus.subscribe(zx::ansi::v2::subscriber_proxy_t{ 5 }.on_target<test_event_t>([](const test_event_t&) {}));

    bus.publish_to(3, test_event_t{});
    EXPECT_THAT(route_builder_calls, testing::Eq(3));

    bus.publish_to(3, test_event_t{});
    EXPECT_THAT(route_builder_calls, testing::Eq(3));

    bus.invalidate_route(3);
    bus.publish_to(3, test_event_t{});
    EXPECT_THAT(route_builder_calls, testing::Eq(6));

    bus.publish_to(5, test_event_t{});
    EXPECT_THAT(route_builder_calls, testing::Eq(8));

    bus.invalidate_all_routes();
    bus.publish_to(3, test_event_t{});
    bus.publish_to(5, test_event_t{});
    EXPECT_THAT(route_builder_calls, testing::Eq(13));
}

TEST(message_bus_v2, context_exposes_current_target_and_phase_for_routed_dispatch)
{
    using namespace zx::ansi::v2;
    message_bus_t bus{ [](message_bus_t::subscriber_id_type target) -> zx::maybe_t<message_bus_t::subscriber_id_type>
                       {
                           switch (target)
                           {
                               case 3: return 2;
                               case 2: return 1;
                               case 1: return zx::none;
                               default: return zx::none;
                           }
                       } };

    std::vector<std::string> calls;

    bus.subscribe(subscriber_proxy_t{ 1 }.on_capture<test_event_t>(
        [&](message_bus_t::context_t& context, const test_event_t&)
        {
            calls.push_back(
                zx::format(*context.subscriber_ids.current_id, ":", *context.subscriber_ids.target_id, ":capture"));

            EXPECT_THAT(
                context,
                testing::AllOf(
                    testing::Field(
                        &message_bus_t::context_t::subscriber_ids,
                        testing::AllOf(
                            testing::Field(
                                &message_bus_t::context_t::subscriber_ids_t::handler_id,
                                testing::Optional(message_bus_t::subscriber_id_type{ 1 })),
                            testing::Field(
                                &message_bus_t::context_t::subscriber_ids_t::current_id,
                                testing::Optional(message_bus_t::subscriber_id_type{ 1 })),
                            testing::Field(
                                &message_bus_t::context_t::subscriber_ids_t::target_id,
                                testing::Optional(message_bus_t::subscriber_id_type{ 3 })))),
                    testing::Field(&message_bus_t::context_t::phase, testing::Optional(event_phase_t::capture))));
        }));

    bus.subscribe(subscriber_proxy_t{ 3 }.on_target<test_event_t>(
        [&](message_bus_t::context_t& context, const test_event_t&)
        {
            calls.push_back(
                zx::format(*context.subscriber_ids.current_id, ":", *context.subscriber_ids.target_id, ":target"));

            EXPECT_THAT(
                context,
                testing::AllOf(
                    testing::Field(
                        &message_bus_t::context_t::subscriber_ids,
                        testing::AllOf(
                            testing::Field(
                                &message_bus_t::context_t::subscriber_ids_t::handler_id,
                                testing::Optional(message_bus_t::subscriber_id_type{ 3 })),
                            testing::Field(
                                &message_bus_t::context_t::subscriber_ids_t::current_id,
                                testing::Optional(message_bus_t::subscriber_id_type{ 3 })),
                            testing::Field(
                                &message_bus_t::context_t::subscriber_ids_t::target_id,
                                testing::Optional(message_bus_t::subscriber_id_type{ 3 })))),
                    testing::Field(&message_bus_t::context_t::phase, testing::Optional(event_phase_t::target))));
        }));

    bus.publish_to(3, test_event_t{});

    EXPECT_THAT(calls, testing::ElementsAre("1:3:capture", "3:3:target"));
}

TEST(message_bus_v2, context_leaves_route_metadata_empty_for_plain_publish)
{
    using namespace zx::ansi::v2;
    message_bus_t bus;

    bus.subscribe(on<test_event_t>(
        [&](message_bus_t::context_t& context, const test_event_t&)
        {
            EXPECT_THAT(
                context,
                testing::AllOf(
                    testing::Field(
                        &message_bus_t::context_t::subscriber_ids,
                        testing::AllOf(
                            testing::Field(&message_bus_t::context_t::subscriber_ids_t::handler_id, testing::Eq(zx::none)),
                            testing::Field(&message_bus_t::context_t::subscriber_ids_t::current_id, testing::Eq(zx::none)),
                            testing::Field(&message_bus_t::context_t::subscriber_ids_t::target_id, testing::Eq(zx::none)))),
                    testing::Field(&message_bus_t::context_t::phase, testing::Eq(zx::none))));
        }));

    bus.publish(test_event_t{});
}

TEST(message_bus_v2, subscribe_and_unsubscribe_are_immediate_outside_publish)
{
    zx::ansi::v2::message_bus_t bus;

    int calls = 0;
    const auto id = bus.subscribe(zx::ansi::v2::on<test_event_t>([&](const test_event_t&) { ++calls; }));

    bus.publish(test_event_t{});
    bus.unsubscribe(id);
    bus.publish(test_event_t{});

    EXPECT_THAT(calls, testing::Eq(1));
}

TEST(message_bus_v2, publish_recovers_after_handler_throws)
{
    using namespace zx::ansi::v2;
    message_bus_t bus;

    bool should_throw = true;
    int calls = 0;

    bus.subscribe(on<test_event_t>(
        [&](message_bus_t::context_t&, const test_event_t&)
        {
            if (should_throw)
            {
                should_throw = false;
                throw std::runtime_error("boom");
            }
            ++calls;
        }));

    try
    {
        bus.publish(test_event_t{});
    }
    catch (const std::runtime_error&)
    {
    }

    bus.subscribe(on<test_event_t>([&](const test_event_t&) { calls += 10; }));
    bus.publish(test_event_t{});

    EXPECT_THAT(calls, testing::Eq(11));
}

TEST(message_bus_v2, unsubscribe_subscriber_removes_only_matching_subscriber)
{
    zx::ansi::v2::message_bus_t bus;

    int calls_a = 0;
    int calls_b = 0;

    bus.subscribe(zx::ansi::v2::subscriber_proxy_t{ 10 }.on<test_event_t>([&](const test_event_t&) { ++calls_a; }));
    bus.subscribe(zx::ansi::v2::subscriber_proxy_t{ 20 }.on<test_event_t>([&](const test_event_t&) { ++calls_b; }));

    bus.publish(test_event_t{});
    bus.unsubscribe_subscriber(10);
    bus.publish(test_event_t{});

    EXPECT_THAT(calls_a, testing::Eq(1));
    EXPECT_THAT(calls_b, testing::Eq(2));
}

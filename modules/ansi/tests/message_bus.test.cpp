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

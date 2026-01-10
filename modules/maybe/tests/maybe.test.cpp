#include <gmock/gmock.h>

#include <zx/maybe.hpp>

TEST(maybe, default_constructed_is_empty)
{
    zx::maybe<int> m;
    EXPECT_FALSE(m.has_value());
}

TEST(maybe, constructed_with_value_is_not_empty)
{
    zx::maybe<int> m{ 42 };
    EXPECT_TRUE(m.has_value());
}

TEST(maybe, constructed_with_none_is_empty)
{
    zx::maybe<int> m{ zx::none };
    EXPECT_FALSE(m.has_value());
}

TEST(maybe, value)
{
    zx::maybe<int> m{ 42 };
    EXPECT_THAT(*m, 42);
    EXPECT_THAT(m.value(), 42);
}

TEST(maybe, value_of_the_maybe_ref)
{
    int value = 42;
    zx::maybe<int&> m{ value };
    EXPECT_THAT(*m, 42);
    EXPECT_THAT(m.value(), 42);
    EXPECT_THAT(*m, testing::Ref(value));
}

TEST(maybe, copy_constructed_maybe_copies_value)
{
    zx::maybe<int> m1{ 42 };
    zx::maybe<int> m2{ m1 };
    EXPECT_TRUE(m2.has_value());
    EXPECT_THAT(*m2, 42);
    EXPECT_TRUE(m1 == m2);
}

TEST(maybe, move_constructed_maybe_moves_value)
{
    zx::maybe<std::string> m1{ "hello" };
    zx::maybe<std::string> m2{ std::move(m1) };
    EXPECT_TRUE(m2.has_value());
    EXPECT_THAT(*m2, "hello");
}

TEST(maybe, maybe_equality)
{
    zx::maybe<int> m1{ 42 };
    zx::maybe<int> m2{ 42 };
    zx::maybe<int> m3{ 43 };
    zx::maybe<int> m4 = zx::none;

    EXPECT_TRUE(m1 == m2);
    EXPECT_FALSE(m1 != m2);
    EXPECT_FALSE(m1 == m3);
    EXPECT_TRUE(m1 != m3);
    EXPECT_FALSE(m1 == m4);
    EXPECT_TRUE(m1 != m4);
    EXPECT_TRUE(m4 == zx::none);
    EXPECT_FALSE(m4 != zx::none);
}

TEST(maybe, maybe_ref_equality)
{
    int value1 = 42;
    int value2 = 42;
    int value3 = 43;

    zx::maybe<int&> m1{ value1 };
    zx::maybe<int&> m2{ value1 };
    zx::maybe<int&> m3{ value2 };
    zx::maybe<int&> m4{ value3 };
    zx::maybe<int&> m5 = zx::none;

    EXPECT_TRUE(m1 == m2);
    EXPECT_FALSE(m1 != m2);
    EXPECT_TRUE(m1 == m3);
    EXPECT_FALSE(m1 != m3);
    EXPECT_FALSE(m1 == m4);
    EXPECT_TRUE(m1 != m4);
    EXPECT_FALSE(m1 == m5);
    EXPECT_TRUE(m1 != m5);
    EXPECT_TRUE(m5 == zx::none);
    EXPECT_FALSE(m5 != zx::none);
}

TEST(maybe, maybe_and_then)
{
    zx::maybe<int> m{ 42 };
    auto m2 = m.and_then([](int value) { return zx::maybe<std::string>{ std::to_string(value) }; });
    EXPECT_TRUE(m2.has_value());
    EXPECT_THAT(*m2, "42");

    zx::maybe<int> empty = zx::none;
    auto m3 = empty.and_then([](int value) { return zx::maybe<std::string>{ std::to_string(value) }; });
    EXPECT_FALSE(m3.has_value());
}

TEST(maybe, maybe_ref_and_then)
{
    int value = 42;
    zx::maybe<int&> m{ value };
    auto m2 = m.and_then([](int& v) { return zx::maybe<std::string>{ std::to_string(v) }; });
    EXPECT_TRUE(m2.has_value());
    EXPECT_THAT(*m2, "42");

    zx::maybe<int&> empty = zx::none;
    auto m3 = empty.and_then([](int& v) { return zx::maybe<std::string>{ std::to_string(v) }; });
    EXPECT_FALSE(m3.has_value());
}

TEST(maybe, maybe_transform)
{
    zx::maybe<int> m{ 42 };
    auto m2 = m.transform([](int value) { return std::to_string(value); });
    EXPECT_TRUE(m2.has_value());
    EXPECT_THAT(*m2, "42");

    zx::maybe<int> empty = zx::none;
    auto m3 = empty.transform([](int value) { return std::to_string(value); });
    EXPECT_FALSE(m3.has_value());
}

TEST(maybe, maybe_ref_transform)
{
    int value = 42;
    zx::maybe<int&> m{ value };
    auto m2 = m.transform([](int& v) { return std::to_string(v); });
    EXPECT_TRUE(m2.has_value());
    EXPECT_THAT(*m2, "42");

    zx::maybe<int&> empty = zx::none;
    auto m3 = empty.transform([](int& v) { return std::to_string(v); });
    EXPECT_FALSE(m3.has_value());
}

TEST(maybe, maybe_value_or)
{
    zx::maybe<int> m{ 42 };
    EXPECT_THAT(m.value_or(0), 42);

    zx::maybe<int> empty = zx::none;
    EXPECT_THAT(empty.value_or(0), 0);
}

TEST(maybe, maybe_ref_value_or)
{
    int value = 42;
    zx::maybe<int&> m{ value };
    EXPECT_THAT(m.value_or(0), 42);

    zx::maybe<int&> empty = zx::none;
    EXPECT_THAT(empty.value_or(0), 0);
}

TEST(maybe, maybe_ref_modification)
{
    int value = 42;
    zx::maybe<int&> m{ value };
    *m = 100;
    EXPECT_THAT(value, 100);
}

TEST(maybe, maybe_no_value_access_throws)
{
    zx::maybe<int> m = zx::none;
    EXPECT_THROW(*m, zx::bad_maybe_access);
    EXPECT_THROW(m.value(), zx::bad_maybe_access);
}

TEST(maybe, maybe_ref_no_value_access_throws)
{
    zx::maybe<int&> m = zx::none;
    EXPECT_THROW(*m, zx::bad_maybe_access);
    EXPECT_THROW(m.value(), zx::bad_maybe_access);
}

TEST(maybe, maybe_ref_assignment_rebinds_the_reference)
{
    int value1 = 42;
    int value2 = 100;
    zx::maybe<int&> m1{ value1 };
    zx::maybe<int&> m2{ value2 };

    m1 = m2;
    EXPECT_TRUE(m1.has_value());
    EXPECT_THAT(*m1, 100);
    EXPECT_THAT(value1, 42);
}

TEST(maybe, maybe_ref_assignment_to_none)
{
    int value = 42;
    zx::maybe<int&> m{ value };

    m = zx::none;
    EXPECT_FALSE(m.has_value());
    EXPECT_THAT(value, 42);
}

TEST(maybe, maybe_ref_assignment_from_none)
{
    zx::maybe<int&> m1 = zx::none;
    int value = 42;
    zx::maybe<int&> m2{ value };

    m1 = m2;
    EXPECT_TRUE(m1.has_value());
    EXPECT_THAT(*m1, 42);
}

TEST(maybe, maybe_ref_filter)
{
    int value = 42;
    zx::maybe<int&> m{ value };

    auto m2 = m.filter([](int v) { return v == 42; });
    EXPECT_TRUE(m2.has_value());
    EXPECT_THAT(*m2, 42);

    auto m3 = m.filter([](int v) { return v != 42; });
    EXPECT_FALSE(m3.has_value());
}

TEST(maybe, maybe_filter)
{
    zx::maybe<int> m{ 42 };

    auto m2 = m.filter([](int v) { return v == 42; });
    EXPECT_TRUE(m2.has_value());
    EXPECT_THAT(*m2, 42);

    auto m3 = m.filter([](int v) { return v != 42; });
    EXPECT_FALSE(m3.has_value());
}

TEST(maybe, maybe_ref_or_else)
{
    int value = 42;
    zx::maybe<int&> m{ value };

    auto m2 = m.or_else([]() { return zx::maybe<int&>{}; });
    EXPECT_TRUE(m2.has_value());
    EXPECT_THAT(*m2, 42);

    zx::maybe<int&> empty = zx::none;
    auto m3 = empty.or_else([&]() { return zx::maybe<int&>{ value }; });
    EXPECT_TRUE(m3.has_value());
    EXPECT_THAT(*m3, 42);
}

TEST(maybe, maybe_or_else)
{
    zx::maybe<int> m{ 42 };

    auto m2 = m.or_else([]() { return zx::maybe<int>{}; });
    EXPECT_TRUE(m2.has_value());
    EXPECT_THAT(*m2, 42);

    zx::maybe<int> empty = zx::none;
    auto m3 = empty.or_else([]() { return zx::maybe<int>{ 100 }; });
    EXPECT_TRUE(m3.has_value());
    EXPECT_THAT(*m3, 100);
}

TEST(maybe, maybe_or_else_void)
{
    zx::maybe<int> m{ 42 };

    bool called = false;
    auto m2 = m.or_else([&]() { called = true; });
    EXPECT_TRUE(m2.has_value());
    EXPECT_THAT(*m2, 42);
    EXPECT_FALSE(called);

    zx::maybe<int> empty = zx::none;
    called = false;
    auto m3 = empty.or_else([&]() { called = true; });
    EXPECT_FALSE(m3.has_value());
    EXPECT_TRUE(called);
}

TEST(maybe, maybe_ref_or_else_void)
{
    int value = 42;
    zx::maybe<int&> m{ value };

    bool called = false;
    auto m2 = m.or_else([&]() { called = true; });
    EXPECT_TRUE(m2.has_value());
    EXPECT_THAT(*m2, 42);
    EXPECT_FALSE(called);

    zx::maybe<int&> empty = zx::none;
    called = false;
    auto m3 = empty.or_else([&]() { called = true; });
    EXPECT_FALSE(m3.has_value());
    EXPECT_TRUE(called);
}

TEST(maybe, maybe_ref_move_and_then)
{
    int value = 42;
    zx::maybe<int&> m{ value };
    auto m2 = std::move(m).and_then([](int& v) { return zx::maybe<std::string>{ std::to_string(v) }; });
    EXPECT_TRUE(m2.has_value());
    EXPECT_THAT(*m2, "42");
}

TEST(maybe, maybe_move_and_then)
{
    zx::maybe<int> m{ 42 };
    auto m2 = std::move(m).and_then([](int value) { return zx::maybe<std::string>{ std::to_string(value) }; });
    EXPECT_TRUE(m2.has_value());
    EXPECT_THAT(*m2, "42");
}

TEST(maybe, maybe_ref_move_transform)
{
    int value = 42;
    zx::maybe<int&> m{ value };
    auto m2 = std::move(m).transform([](int& v) { return std::to_string(v); });
    EXPECT_TRUE(m2.has_value());
    EXPECT_THAT(*m2, "42");
}

TEST(maybe, maybe_move_transform)
{
    zx::maybe<int> m{ 42 };
    auto m2 = std::move(m).transform([](int value) { return std::to_string(value); });
    EXPECT_TRUE(m2.has_value());
    EXPECT_THAT(*m2, "42");
}

TEST(maybe, maybe_move_value_or)
{
    zx::maybe<int> m{ 42 };
    EXPECT_THAT(std::move(m).value_or(0), 42);

    zx::maybe<int> empty = zx::none;
    EXPECT_THAT(std::move(empty).value_or(0), 0);
}

TEST(maybe, maybe_ref_move_value_or)
{
    int value = 42;
    zx::maybe<int&> m{ value };
    EXPECT_THAT(std::move(m).value_or(0), 42);

    zx::maybe<int&> empty = zx::none;
    EXPECT_THAT(std::move(empty).value_or(0), 0);
}

TEST(maybe, maybe_move_no_value_access_throws)
{
    zx::maybe<int> m = zx::none;
    EXPECT_THROW(std::move(m).value(), zx::bad_maybe_access);
}

TEST(maybe, maybe_ref_move_no_value_access_throws)
{
    zx::maybe<int&> m = zx::none;
    EXPECT_THROW(std::move(m).value(), zx::bad_maybe_access);
}

TEST(maybe, maybe_ref_move_assignment_rebinds_the_reference)
{
    int value1 = 42;
    int value2 = 100;
    zx::maybe<int&> m1{ value1 };
    zx::maybe<int&> m2{ value2 };

    m1 = std::move(m2);
    EXPECT_TRUE(m1.has_value());
    EXPECT_THAT(*m1, 100);
    EXPECT_THAT(value1, 42);
}

TEST(maybe, maybe_ref_move_assignment_to_none)
{
    int value = 42;
    zx::maybe<int&> m{ value };

    m = zx::none;
    EXPECT_FALSE(m.has_value());
    EXPECT_THAT(value, 42);
}

TEST(maybe, maybe_ref_move_assignment_from_none)
{
    zx::maybe<int&> m1 = zx::none;
    int value = 42;
    zx::maybe<int&> m2{ value };

    m1 = std::move(m2);
    EXPECT_TRUE(m1.has_value());
    EXPECT_THAT(*m1, 42);
}

TEST(maybe, maybe_ref_move_filter)
{
    int value = 42;
    zx::maybe<int&> m{ value };

    auto m2 = std::move(m).filter([](int v) { return v == 42; });
    EXPECT_TRUE(m2.has_value());
    EXPECT_THAT(*m2, 42);
}

TEST(maybe, maybe_move_filter)
{
    zx::maybe<int> m{ 42 };

    auto m2 = std::move(m).filter([](int v) { return v == 42; });
    EXPECT_TRUE(m2.has_value());
    EXPECT_THAT(*m2, 42);
}

TEST(maybe, maybe_ref_move_or_else)
{
    int value = 42;
    zx::maybe<int&> m{ value };

    auto m2 = std::move(m).or_else([]() { return zx::maybe<int&>{}; });
    EXPECT_TRUE(m2.has_value());
    EXPECT_THAT(*m2, 42);
}

TEST(maybe, maybe_move_or_else)
{
    zx::maybe<int> m{ 42 };

    auto m2 = std::move(m).or_else([]() { return zx::maybe<int>{}; });
    EXPECT_TRUE(m2.has_value());
    EXPECT_THAT(*m2, 42);
}

TEST(maybe, maybe_ref_move_or_else_void)
{
    int value = 42;
    zx::maybe<int&> m{ value };

    bool called = false;
    auto m2 = std::move(m).or_else([&]() { called = true; });
    EXPECT_TRUE(m2.has_value());
    EXPECT_THAT(*m2, 42);
    EXPECT_FALSE(called);
}

TEST(maybe, maybe_move_or_else_void)
{
    zx::maybe<int> m{ 42 };

    bool called = false;
    auto m2 = std::move(m).or_else([&]() { called = true; });
    EXPECT_TRUE(m2.has_value());
    EXPECT_THAT(*m2, 42);
    EXPECT_FALSE(called);
}

TEST(maybe, bad_maybe_access_what)
{
    zx::bad_maybe_access ex;
    EXPECT_STREQ(ex.what(), "zx::bad_maybe_access");
}

TEST(maybe, maybe_ref_filter_no_value)
{
    zx::maybe<int&> m = zx::none;

    auto m2 = m.filter([](int v) { return v == 42; });
    EXPECT_FALSE(m2.has_value());
}

TEST(maybe, maybe_filter_no_value)
{
    zx::maybe<int> m = zx::none;

    auto m2 = m.filter([](int v) { return v == 42; });
    EXPECT_FALSE(m2.has_value());
}

TEST(maybe, maybe_ref_move_filter_no_value)
{
    zx::maybe<int&> m = zx::none;

    auto m2 = std::move(m).filter([](int v) { return v == 42; });
    EXPECT_FALSE(m2.has_value());
}

TEST(maybe, maybe_move_filter_no_value)
{
    zx::maybe<int> m = zx::none;

    auto m2 = std::move(m).filter([](int v) { return v == 42; });
    EXPECT_FALSE(m2.has_value());
}

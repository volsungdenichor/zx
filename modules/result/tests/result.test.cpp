#include <gmock/gmock.h>

#include <zx/result.hpp>

namespace
{

std::string get_error_message(const std::exception_ptr& ptr)
{
    try
    {
        if (ptr)
        {
            std::rethrow_exception(ptr);
        }
    }
    catch (const std::exception& ex)
    {
        return ex.what();
    }
    return {};
}

}  // namespace

TEST(result, default_constructed_has_value)
{
    zx::result<int, std::string> r;
    EXPECT_TRUE(r.has_value());
    EXPECT_THAT(*r, 0);
    EXPECT_FALSE(r.has_error());
}

TEST(result, constructed_with_error_has_error)
{
    zx::result<int, std::string> r{ zx::error("error occurred") };
    EXPECT_FALSE(r.has_value());
    EXPECT_TRUE(r.has_error());
    EXPECT_THAT(r.error(), "error occurred");
}

TEST(result, constructed_with_value)
{
    zx::result<int, std::string> r{ 42 };
    EXPECT_TRUE(r.has_value());
    EXPECT_THAT(*r, 42);
    EXPECT_FALSE(r.has_error());
}

TEST(result, copy_constructor_with_value)
{
    zx::result<int, std::string> r1{ 42 };
    zx::result<int, std::string> r2{ r1 };
    EXPECT_TRUE(r2.has_value());
    EXPECT_THAT(*r2, 42);
}

TEST(result, copy_constructor_with_error)
{
    zx::result<int, std::string> r1{ zx::error("failed") };
    zx::result<int, std::string> r2{ r1 };
    EXPECT_TRUE(r2.has_error());
    EXPECT_THAT(r2.error(), "failed");
}

TEST(result, move_constructor_with_value)
{
    zx::result<int, std::string> r1{ 42 };
    zx::result<int, std::string> r2{ std::move(r1) };
    EXPECT_TRUE(r2.has_value());
    EXPECT_THAT(*r2, 42);
}

TEST(result, move_constructor_with_error)
{
    zx::result<int, std::string> r1{ zx::error("failed") };
    zx::result<int, std::string> r2{ std::move(r1) };
    EXPECT_TRUE(r2.has_error());
    EXPECT_THAT(r2.error(), "failed");
}

TEST(result, copy_assignment_with_value)
{
    zx::result<int, std::string> r1{ 42 };
    zx::result<int, std::string> r2;
    r2 = r1;
    EXPECT_TRUE(r2.has_value());
    EXPECT_THAT(*r2, 42);
}

TEST(result, copy_assignment_with_error)
{
    zx::result<int, std::string> r1{ zx::error("failed") };
    zx::result<int, std::string> r2;
    r2 = r1;
    EXPECT_TRUE(r2.has_error());
    EXPECT_THAT(r2.error(), "failed");
}

TEST(result, move_assignment_with_value)
{
    zx::result<int, std::string> r1{ 42 };
    zx::result<int, std::string> r2;
    r2 = std::move(r1);
    EXPECT_TRUE(r2.has_value());
    EXPECT_THAT(*r2, 42);
}

TEST(result, move_assignment_with_error)
{
    zx::result<int, std::string> r1{ zx::error("failed") };
    zx::result<int, std::string> r2;
    r2 = std::move(r1);
    EXPECT_TRUE(r2.has_error());
    EXPECT_THAT(r2.error(), "failed");
}

TEST(result, arrow_operator_access)
{
    zx::result<std::string, int> r{ "hello" };
    EXPECT_THAT(r->size(), 5);
}

TEST(result, const_dereference)
{
    const zx::result<int, std::string> r{ 42 };
    EXPECT_THAT(*r, 42);
}

TEST(result, value_method)
{
    zx::result<int, std::string> r{ 42 };
    EXPECT_THAT(r.value(), 42);
}

TEST(result, const_value_method)
{
    const zx::result<int, std::string> r{ 42 };
    EXPECT_THAT(r.value(), 42);
}

TEST(result, const_error_method)
{
    const zx::result<int, std::string> r{ zx::error("failed") };
    EXPECT_THAT(r.error(), "failed");
}

TEST(result, bool_conversion_with_value)
{
    zx::result<int, std::string> r{ 42 };
    EXPECT_TRUE(static_cast<bool>(r));
}

TEST(result, bool_conversion_with_error)
{
    zx::result<int, std::string> r{ zx::error("failed") };
    EXPECT_FALSE(static_cast<bool>(r));
}

TEST(result, assign_value_over_value)
{
    zx::result<int, std::string> r{ 10 };
    r = 20;
    EXPECT_TRUE(r.has_value());
    EXPECT_THAT(*r, 20);
}

TEST(result, assign_value_over_error)
{
    zx::result<int, std::string> r{ zx::error("failed") };
    r = 42;
    EXPECT_TRUE(r.has_value());
    EXPECT_THAT(*r, 42);
}

TEST(result, assign_error_over_value)
{
    zx::result<int, std::string> r{ 42 };
    r = zx::error("new error");
    EXPECT_TRUE(r.has_error());
    EXPECT_THAT(r.error(), "new error");
}

TEST(result, assign_error_over_error)
{
    zx::result<int, std::string> r{ zx::error("old error") };
    r = zx::error("new error");
    EXPECT_TRUE(r.has_error());
    EXPECT_THAT(r.error(), "new error");
}

TEST(result_ref, constructed_with_reference)
{
    int value = 42;
    zx::result<int&, std::string> r{ value };
    EXPECT_TRUE(r.has_value());
    EXPECT_THAT(*r, 42);
    EXPECT_FALSE(r.has_error());
}

TEST(result_ref, constructed_with_error)
{
    zx::result<int&, std::string> r{ zx::error("failed") };
    EXPECT_FALSE(r.has_value());
    EXPECT_TRUE(r.has_error());
    EXPECT_THAT(r.error(), "failed");
}

TEST(result_ref, reference_modification)
{
    int value = 42;
    zx::result<int&, std::string> r{ value };
    *r = 100;
    EXPECT_THAT(value, 100);
    EXPECT_THAT(*r, 100);
}

TEST(result_ref, copy_constructor_with_reference)
{
    int value = 42;
    zx::result<int&, std::string> r1{ value };
    zx::result<int&, std::string> r2{ r1 };
    EXPECT_TRUE(r2.has_value());
    EXPECT_THAT(*r2, 42);
    *r2 = 50;
    EXPECT_THAT(value, 50);
}

TEST(result_ref, copy_constructor_with_error)
{
    zx::result<int&, std::string> r1{ zx::error("failed") };
    zx::result<int&, std::string> r2{ r1 };
    EXPECT_TRUE(r2.has_error());
    EXPECT_THAT(r2.error(), "failed");
}

TEST(result_ref, move_constructor_with_reference)
{
    int value = 42;
    zx::result<int&, std::string> r1{ value };
    zx::result<int&, std::string> r2{ std::move(r1) };
    EXPECT_TRUE(r2.has_value());
    EXPECT_THAT(*r2, 42);
}

TEST(result_ref, move_constructor_with_error)
{
    zx::result<int&, std::string> r1{ zx::error("failed") };
    zx::result<int&, std::string> r2{ std::move(r1) };
    EXPECT_TRUE(r2.has_error());
    EXPECT_THAT(r2.error(), "failed");
}

TEST(result_ref, copy_assignment_with_reference)
{
    int value1 = 42;
    int value2 = 10;
    zx::result<int&, std::string> r1{ value1 };
    zx::result<int&, std::string> r2{ value2 };
    r2 = r1;
    EXPECT_TRUE(r2.has_value());
    EXPECT_THAT(*r2, 42);
    *r2 = 99;
    EXPECT_THAT(value1, 99);
}

TEST(result_ref, copy_assignment_with_error)
{
    int value = 10;
    zx::result<int&, std::string> r1{ zx::error("failed") };
    zx::result<int&, std::string> r2{ value };
    r2 = r1;
    EXPECT_TRUE(r2.has_error());
    EXPECT_THAT(r2.error(), "failed");
}

TEST(result_ref, move_assignment_with_reference)
{
    int value1 = 42;
    int value2 = 10;
    zx::result<int&, std::string> r1{ value1 };
    zx::result<int&, std::string> r2{ value2 };
    r2 = std::move(r1);
    EXPECT_TRUE(r2.has_value());
    EXPECT_THAT(*r2, 42);
}

TEST(result_ref, move_assignment_with_error)
{
    int value = 10;
    zx::result<int&, std::string> r1{ zx::error("failed") };
    zx::result<int&, std::string> r2{ value };
    r2 = std::move(r1);
    EXPECT_TRUE(r2.has_error());
    EXPECT_THAT(r2.error(), "failed");
}

TEST(result_ref, arrow_operator_access)
{
    std::string str = "hello";
    zx::result<std::string&, int> r{ str };
    EXPECT_THAT(r->size(), 5);
}

TEST(result_ref, const_dereference)
{
    int value = 42;
    const zx::result<int&, std::string> r{ value };
    EXPECT_THAT(*r, 42);
}

TEST(result_ref, value_method)
{
    int value = 42;
    zx::result<int&, std::string> r{ value };
    EXPECT_THAT(r.value(), 42);
    r.value() = 100;
    EXPECT_THAT(value, 100);
}

TEST(result_ref, const_value_method)
{
    int value = 42;
    const zx::result<int&, std::string> r{ value };
    EXPECT_THAT(r.value(), 42);
}

TEST(result_ref, bool_conversion_with_reference)
{
    int value = 42;
    zx::result<int&, std::string> r{ value };
    EXPECT_TRUE(static_cast<bool>(r));
}

TEST(result_ref, bool_conversion_with_error)
{
    zx::result<int&, std::string> r{ zx::error("failed") };
    EXPECT_FALSE(static_cast<bool>(r));
}

TEST(result_ref, assign_reference_over_reference)
{
    int value1 = 42;
    int value2 = 100;
    zx::result<int&, std::string> r{ value1 };
    r = value2;
    EXPECT_TRUE(r.has_value());
    EXPECT_THAT(*r, 100);
    *r = 200;
    EXPECT_THAT(value2, 200);
}

TEST(result_ref, assign_reference_over_error)
{
    int value = 42;
    zx::result<int&, std::string> r{ zx::error("failed") };
    r = value;
    EXPECT_TRUE(r.has_value());
    EXPECT_THAT(*r, 42);
}

TEST(result_ref, assign_error_over_reference)
{
    int value = 42;
    zx::result<int&, std::string> r{ value };
    r = zx::error("new error");
    EXPECT_TRUE(r.has_error());
    EXPECT_THAT(r.error(), "new error");
}

TEST(result_ref, assign_error_over_error)
{
    zx::result<int&, std::string> r{ zx::error("old error") };
    r = zx::error("new error");
    EXPECT_TRUE(r.has_error());
    EXPECT_THAT(r.error(), "new error");
}

TEST(result_void, default_constructed_has_value)
{
    zx::result<void, std::string> r;
    EXPECT_TRUE(r.has_value());
    EXPECT_FALSE(r.has_error());
}

TEST(result_void, constructed_with_error_has_error)
{
    zx::result<void, std::string> r{ zx::error("error occurred") };
    EXPECT_FALSE(r.has_value());
    EXPECT_TRUE(r.has_error());
    EXPECT_THAT(r.error(), "error occurred");
}

TEST(result_void, copy_constructor_with_value)
{
    zx::result<void, std::string> r1;
    zx::result<void, std::string> r2{ r1 };
    EXPECT_TRUE(r2.has_value());
}

TEST(result_void, copy_constructor_with_error)
{
    zx::result<void, std::string> r1{ zx::error("failed") };
    zx::result<void, std::string> r2{ r1 };
    EXPECT_TRUE(r2.has_error());
    EXPECT_THAT(r2.error(), "failed");
}

TEST(result_void, move_constructor_with_value)
{
    zx::result<void, std::string> r1;
    zx::result<void, std::string> r2{ std::move(r1) };
    EXPECT_TRUE(r2.has_value());
}

TEST(result_void, move_constructor_with_error)
{
    zx::result<void, std::string> r1{ zx::error("failed") };
    zx::result<void, std::string> r2{ std::move(r1) };
    EXPECT_TRUE(r2.has_error());
    EXPECT_THAT(r2.error(), "failed");
}

TEST(result_void, copy_assignment_with_value)
{
    zx::result<void, std::string> r1;
    zx::result<void, std::string> r2{ zx::error("error") };
    r2 = r1;
    EXPECT_TRUE(r2.has_value());
}

TEST(result_void, copy_assignment_with_error)
{
    zx::result<void, std::string> r1{ zx::error("failed") };
    zx::result<void, std::string> r2;
    r2 = r1;
    EXPECT_TRUE(r2.has_error());
    EXPECT_THAT(r2.error(), "failed");
}

TEST(result_void, move_assignment_with_value)
{
    zx::result<void, std::string> r1;
    zx::result<void, std::string> r2{ zx::error("error") };
    r2 = std::move(r1);
    EXPECT_TRUE(r2.has_value());
}

TEST(result_void, move_assignment_with_error)
{
    zx::result<void, std::string> r1{ zx::error("failed") };
    zx::result<void, std::string> r2;
    r2 = std::move(r1);
    EXPECT_TRUE(r2.has_error());
    EXPECT_THAT(r2.error(), "failed");
}

TEST(result_void, const_error_method)
{
    const zx::result<void, std::string> r{ zx::error("failed") };
    EXPECT_THAT(r.error(), "failed");
}

TEST(result_void, bool_conversion_with_value)
{
    zx::result<void, std::string> r;
    EXPECT_TRUE(static_cast<bool>(r));
}

TEST(result_void, bool_conversion_with_error)
{
    zx::result<void, std::string> r{ zx::error("failed") };
    EXPECT_FALSE(static_cast<bool>(r));
}

TEST(result_void, assign_error_over_value)
{
    zx::result<void, std::string> r;
    r = zx::error("new error");
    EXPECT_TRUE(r.has_error());
    EXPECT_THAT(r.error(), "new error");
}

TEST(result_void, assign_error_over_error)
{
    zx::result<void, std::string> r{ zx::error("old error") };
    r = zx::error("new error");
    EXPECT_TRUE(r.has_error());
    EXPECT_THAT(r.error(), "new error");
}

TEST(result, and_then_with_value)
{
    zx::result<int, std::string> r{ 42 };
    auto result = r.and_then([](int value) { return zx::result<int, std::string>{ value * 2 }; });
    EXPECT_TRUE(result.has_value());
    EXPECT_THAT(*result, 84);
}

TEST(result, and_then_with_error)
{
    zx::result<int, std::string> r{ zx::error("failed") };
    auto result = r.and_then([](int value) { return zx::result<int, std::string>{ value * 2 }; });
    EXPECT_TRUE(result.has_error());
    EXPECT_THAT(result.error(), "failed");
}

TEST(result, and_then_returns_error)
{
    zx::result<int, std::string> r{ 42 };
    auto result = r.and_then([](int) { return zx::result<int, std::string>{ zx::error("new error") }; });
    EXPECT_TRUE(result.has_error());
    EXPECT_THAT(result.error(), "new error");
}

TEST(result, and_then_chain)
{
    zx::result<int, std::string> r{ 10 };
    auto result = r.and_then([](int value) { return zx::result<int, std::string>{ value + 5 }; })
                      .and_then([](int value) { return zx::result<int, std::string>{ value * 2 }; });
    EXPECT_TRUE(result.has_value());
    EXPECT_THAT(*result, 30);
}

TEST(result, and_then_different_type)
{
    zx::result<int, std::string> r{ 42 };
    auto result = r.and_then([](int value) { return zx::result<std::string, std::string>{ std::to_string(value) }; });
    EXPECT_TRUE(result.has_value());
    EXPECT_THAT(*result, "42");
}

TEST(result_ref, and_then_with_reference)
{
    int value = 42;
    zx::result<int&, std::string> r{ value };
    auto result = r.and_then([](int& v) { return zx::result<int, std::string>{ v * 2 }; });
    EXPECT_TRUE(result.has_value());
    EXPECT_THAT(*result, 84);
}

TEST(result_ref, and_then_with_error)
{
    zx::result<int&, std::string> r{ zx::error("failed") };
    auto result = r.and_then([](int& v) { return zx::result<int, std::string>{ v * 2 }; });
    EXPECT_TRUE(result.has_error());
    EXPECT_THAT(result.error(), "failed");
}

TEST(result_ref, and_then_returns_error)
{
    int value = 42;
    zx::result<int&, std::string> r{ value };
    auto result = r.and_then([](int&) { return zx::result<int, std::string>{ zx::error("new error") }; });
    EXPECT_TRUE(result.has_error());
    EXPECT_THAT(result.error(), "new error");
}

TEST(result_ref, and_then_chain)
{
    int value = 10;
    zx::result<int&, std::string> r{ value };
    auto result = r.and_then([](int& v) { return zx::result<int, std::string>{ v + 5 }; })
                      .and_then([](int v) { return zx::result<int, std::string>{ v * 2 }; });
    EXPECT_TRUE(result.has_value());
    EXPECT_THAT(*result, 30);
}

TEST(result_void, and_then_with_value)
{
    zx::result<void, std::string> r;
    auto result = r.and_then([]() { return zx::result<int, std::string>{ 42 }; });
    EXPECT_TRUE(result.has_value());
    EXPECT_THAT(*result, 42);
}

TEST(result_void, and_then_with_error)
{
    zx::result<void, std::string> r{ zx::error("failed") };
    auto result = r.and_then([]() { return zx::result<int, std::string>{ 42 }; });
    EXPECT_TRUE(result.has_error());
    EXPECT_THAT(result.error(), "failed");
}

TEST(result_void, and_then_returns_error)
{
    zx::result<void, std::string> r;
    auto result = r.and_then([]() { return zx::result<int, std::string>{ zx::error("new error") }; });
    EXPECT_TRUE(result.has_error());
    EXPECT_THAT(result.error(), "new error");
}

TEST(result_void, and_then_chain)
{
    zx::result<void, std::string> r;
    auto result = r.and_then([]() { return zx::result<int, std::string>{ 10 }; })
                      .and_then([](int value) { return zx::result<int, std::string>{ value * 2 }; });
    EXPECT_TRUE(result.has_value());
    EXPECT_THAT(*result, 20);
}

TEST(result_void, and_then_to_void)
{
    zx::result<void, std::string> r;
    auto result = r.and_then([]() { return zx::result<void, std::string>{}; });
    EXPECT_TRUE(result.has_value());
}

TEST(result_void, and_then_to_void_with_error)
{
    zx::result<void, std::string> r{ zx::error("old error") };
    auto result = r.and_then([]() { return zx::result<void, std::string>{ zx::error("new error") }; });
    EXPECT_TRUE(result.has_error());
    EXPECT_THAT(result.error(), "old error");
}

TEST(result_void, and_then_from_void_to_value)
{
    zx::result<void, std::string> r;
    auto result = r.and_then([]() { return zx::result<int, std::string>{ 42 }; });
    EXPECT_TRUE(result.has_value());
    EXPECT_THAT(*result, 42);
}

TEST(result, transform_with_value)
{
    zx::result<int, std::string> r{ 42 };
    auto result = r.transform([](int value) { return value * 2; });
    EXPECT_TRUE(result.has_value());
    EXPECT_THAT(*result, 84);
}

TEST(result, transform_with_error)
{
    zx::result<int, std::string> r{ zx::error("failed") };
    auto result = r.transform([](int value) { return value * 2; });
    EXPECT_TRUE(result.has_error());
    EXPECT_THAT(result.error(), "failed");
}

TEST(result, transform_chain)
{
    zx::result<int, std::string> r{ 10 };
    auto result = r.transform([](int value) { return value + 5; }).transform([](int value) { return value * 2; });
    EXPECT_TRUE(result.has_value());
    EXPECT_THAT(*result, 30);
}

TEST(result, transform_different_type)
{
    zx::result<int, std::string> r{ 42 };
    auto result = r.transform([](int value) { return std::to_string(value); });
    EXPECT_TRUE(result.has_value());
    EXPECT_THAT(*result, "42");
}

TEST(result, transform_to_void)
{
    zx::result<int, std::string> r{ 42 };
    int side_effect = 0;
    auto result = r.transform([&side_effect](int value) { side_effect = value; });
    EXPECT_TRUE(result.has_value());
    EXPECT_THAT(side_effect, 42);
}

TEST(result, or_else_with_value)
{
    zx::result<int, std::string> r{ 42 };
    auto result = r.or_else([](const std::string&) { return zx::result<int, std::string>{ 0 }; });
    EXPECT_TRUE(result.has_value());
    EXPECT_THAT(*result, 42);
}

TEST(result, or_else_with_error)
{
    zx::result<int, std::string> r{ zx::error("failed") };
    auto result = r.or_else([](const std::string& err) { return zx::result<int, std::string>{ 100 }; });
    EXPECT_TRUE(result.has_value());
    EXPECT_THAT(*result, 100);
}

TEST(result, or_else_returns_error)
{
    zx::result<int, std::string> r{ zx::error("failed") };
    auto result = r.or_else([](const std::string&) { return zx::result<int, std::string>{ zx::error("new error") }; });
    EXPECT_TRUE(result.has_error());
    EXPECT_THAT(result.error(), "new error");
}

TEST(result, or_else_chain)
{
    zx::result<int, std::string> r{ zx::error("failed") };
    auto result = r.or_else([](const std::string&) { return zx::result<int, std::string>{ zx::error("error2") }; })
                      .or_else([](const std::string&) { return zx::result<int, std::string>{ 50 }; });
    EXPECT_TRUE(result.has_value());
    EXPECT_THAT(*result, 50);
}

TEST(result_ref, transform_chain)
{
    int value = 10;
    zx::result<int&, std::string> r{ value };
    auto result = r.transform([](int v) { return v + 5; }).transform([](int v) { return v * 2; });
    EXPECT_TRUE(result.has_value());
    EXPECT_THAT(*result, 30);
}

TEST(result_ref, transform_different_type)
{
    int value = 42;
    zx::result<int&, std::string> r{ value };
    auto result = r.transform([](int& v) { return std::to_string(v); });
    EXPECT_TRUE(result.has_value());
    EXPECT_THAT(*result, "42");
}

TEST(result_ref, or_else_with_reference)
{
    int value = 42;
    zx::result<int&, std::string> r{ value };
    auto result = r.or_else(
        [](const std::string&)
        {
            static int fallback = 0;
            return zx::result<int&, std::string>{ fallback };
        });
    EXPECT_TRUE(result.has_value());
    EXPECT_THAT(*result, 42);
}

TEST(result_ref, or_else_with_error)
{
    zx::result<int&, std::string> r{ zx::error("failed") };
    auto result = r.or_else(
        [](const std::string& err)
        {
            static int fallback = 100;
            return zx::result<int&, std::string>{ fallback };
        });
    EXPECT_TRUE(result.has_value());
    EXPECT_THAT(*result, 100);
}

TEST(result_ref, or_else_returns_error)
{
    zx::result<int&, std::string> r{ zx::error("failed") };
    auto result = r.or_else([](const std::string&) { return zx::result<int&, std::string>{ zx::error("new error") }; });
    EXPECT_TRUE(result.has_error());
    EXPECT_THAT(result.error(), "new error");
}

TEST(result_void, transform_with_value)
{
    zx::result<void, std::string> r;
    auto result = r.transform([]() { return 42; });
    EXPECT_TRUE(result.has_value());
    EXPECT_THAT(*result, 42);
}

TEST(result_void, transform_with_error)
{
    zx::result<void, std::string> r{ zx::error("failed") };
    auto result = r.transform([]() { return 42; });
    EXPECT_TRUE(result.has_error());
    EXPECT_THAT(result.error(), "failed");
}

TEST(result_void, transform_chain)
{
    zx::result<void, std::string> r;
    auto result = r.transform([]() { return 10; }).transform([](int value) { return value * 2; });
    EXPECT_TRUE(result.has_value());
    EXPECT_THAT(*result, 20);
}

TEST(result_void, transform_to_void)
{
    zx::result<void, std::string> r;
    int side_effect = 0;
    auto result = r.transform([&side_effect]() { side_effect = 42; });
    EXPECT_TRUE(result.has_value());
    EXPECT_THAT(side_effect, 42);
}

TEST(result_void, or_else_with_value)
{
    zx::result<void, std::string> r;
    auto result = r.or_else([](const std::string&) { return zx::result<void, std::string>{}; });
    EXPECT_TRUE(result.has_value());
}

TEST(result_void, or_else_with_error)
{
    zx::result<void, std::string> r{ zx::error("failed") };
    auto result = r.or_else([](const std::string& err) { return zx::result<void, std::string>{}; });
    EXPECT_TRUE(result.has_value());
}

TEST(result_void, or_else_returns_error)
{
    zx::result<void, std::string> r{ zx::error("failed") };
    auto result = r.or_else([](const std::string&) { return zx::result<void, std::string>{ zx::error("new error") }; });
    EXPECT_TRUE(result.has_error());
    EXPECT_THAT(result.error(), "new error");
}

TEST(result, transform_error_with_value)
{
    zx::result<int, std::string> r{ 42 };
    auto result = r.transform_error([](const std::string& err) { return err + " transformed"; });
    EXPECT_TRUE(result.has_value());
    EXPECT_THAT(*result, 42);
}

TEST(result, transform_error_with_error)
{
    zx::result<int, std::string> r{ zx::error("failed") };
    auto result = r.transform_error([](const std::string& err) { return err + " transformed"; });
    EXPECT_TRUE(result.has_error());
    EXPECT_THAT(result.error(), "failed transformed");
}

TEST(result, transform_error_chain)
{
    zx::result<int, std::string> r{ zx::error("error") };
    auto result = r.transform_error([](const std::string& err) { return err + " step1"; })
                      .transform_error([](const std::string& err) { return err + " step2"; });
    EXPECT_TRUE(result.has_error());
    EXPECT_THAT(result.error(), "error step1 step2");
}

TEST(result, transform_error_different_type)
{
    zx::result<int, std::string> r{ zx::error("failed") };
    auto result = r.transform_error([](const std::string& err) { return static_cast<int>(err.size()); });
    EXPECT_TRUE(result.has_error());
    EXPECT_THAT(result.error(), 6);
}

TEST(result_ref, transform_error_with_reference)
{
    int value = 42;
    zx::result<int&, std::string> r{ value };
    auto result = r.transform_error([](const std::string& err) { return err + " transformed"; });
    EXPECT_TRUE(result.has_value());
    EXPECT_THAT(*result, 42);
}

TEST(result_ref, transform_error_with_error)
{
    zx::result<int&, std::string> r{ zx::error("failed") };
    auto result = r.transform_error([](const std::string& err) { return err + " transformed"; });
    EXPECT_TRUE(result.has_error());
    EXPECT_THAT(result.error(), "failed transformed");
}

TEST(result_ref, transform_error_chain)
{
    zx::result<int&, std::string> r{ zx::error("error") };
    auto result = r.transform_error([](const std::string& err) { return err + " step1"; })
                      .transform_error([](const std::string& err) { return err + " step2"; });
    EXPECT_TRUE(result.has_error());
    EXPECT_THAT(result.error(), "error step1 step2");
}

TEST(result_ref, transform_error_different_type)
{
    zx::result<int&, std::string> r{ zx::error("failed") };
    auto result = r.transform_error([](const std::string& err) { return static_cast<int>(err.size()); });
    EXPECT_TRUE(result.has_error());
    EXPECT_THAT(result.error(), 6);
}

TEST(result_void, transform_error_with_value)
{
    zx::result<void, std::string> r;
    auto result = r.transform_error([](const std::string& err) { return err + " transformed"; });
    EXPECT_TRUE(result.has_value());
}

TEST(result_void, transform_error_with_error)
{
    zx::result<void, std::string> r{ zx::error("failed") };
    auto result = r.transform_error([](const std::string& err) { return err + " transformed"; });
    EXPECT_TRUE(result.has_error());
    EXPECT_THAT(result.error(), "failed transformed");
}

TEST(result_void, transform_error_chain)
{
    zx::result<void, std::string> r{ zx::error("error") };
    auto result = r.transform_error([](const std::string& err) { return err + " step1"; })
                      .transform_error([](const std::string& err) { return err + " step2"; });
    EXPECT_TRUE(result.has_error());
    EXPECT_THAT(result.error(), "error step1 step2");
}

TEST(result_void, transform_error_different_type)
{
    zx::result<void, std::string> r{ zx::error("failed") };
    auto result = r.transform_error([](const std::string& err) { return static_cast<int>(err.size()); });
    EXPECT_TRUE(result.has_error());
    EXPECT_THAT(result.error(), 6);
}

TEST(result, value_or_with_value)
{
    zx::result<int, std::string> r{ 42 };
    EXPECT_THAT(r.value_or(100), 42);
}

TEST(result, value_or_with_error)
{
    zx::result<int, std::string> r{ zx::error("failed") };
    EXPECT_THAT(r.value_or(100), 100);
}

TEST(result, value_or_const_with_value)
{
    const zx::result<int, std::string> r{ 42 };
    EXPECT_THAT(r.value_or(100), 42);
}

TEST(result, value_or_const_with_error)
{
    const zx::result<int, std::string> r{ zx::error("failed") };
    EXPECT_THAT(r.value_or(100), 100);
}

TEST(result, value_or_rvalue_with_value)
{
    EXPECT_THAT((zx::result<int, std::string>{ 42 }.value_or(100)), 42);
}

TEST(result, value_or_rvalue_with_error)
{
    EXPECT_THAT((zx::result<int, std::string>{ zx::error("failed") }.value_or(100)), 100);
}

TEST(result_ref, value_or_with_reference)
{
    int value = 42;
    int fallback = 100;
    zx::result<int&, std::string> r{ value };
    EXPECT_THAT(r.value_or(fallback), 42);
}

TEST(result_ref, value_or_with_error)
{
    int fallback = 100;
    zx::result<int&, std::string> r{ zx::error("failed") };
    EXPECT_THAT(r.value_or(fallback), 100);
}

TEST(result_ref, value_or_const_with_reference)
{
    int value = 42;
    int fallback = 100;
    const zx::result<int&, std::string> r{ value };
    EXPECT_THAT(r.value_or(fallback), 42);
}

TEST(result_ref, value_or_const_with_error)
{
    int fallback = 100;
    const zx::result<int&, std::string> r{ zx::error("failed") };
    EXPECT_THAT(r.value_or(fallback), 100);
}

TEST(result, equality_both_values_equal)
{
    zx::result<int, std::string> r1{ 42 };
    zx::result<int, std::string> r2{ 42 };
    EXPECT_TRUE(r1 == r2);
    EXPECT_FALSE(r1 != r2);
}

TEST(result, equality_both_values_not_equal)
{
    zx::result<int, std::string> r1{ 42 };
    zx::result<int, std::string> r2{ 100 };
    EXPECT_FALSE(r1 == r2);
    EXPECT_TRUE(r1 != r2);
}

TEST(result, equality_both_errors_equal)
{
    zx::result<int, std::string> r1{ zx::error("failed") };
    zx::result<int, std::string> r2{ zx::error("failed") };
    EXPECT_TRUE(r1 == r2);
    EXPECT_FALSE(r1 != r2);
}

TEST(result, equality_both_errors_not_equal)
{
    zx::result<int, std::string> r1{ zx::error("failed") };
    zx::result<int, std::string> r2{ zx::error("other error") };
    EXPECT_FALSE(r1 == r2);
    EXPECT_TRUE(r1 != r2);
}

TEST(result, equality_value_vs_error)
{
    zx::result<int, std::string> r1{ 42 };
    zx::result<int, std::string> r2{ zx::error("failed") };
    EXPECT_FALSE(r1 == r2);
    EXPECT_TRUE(r1 != r2);
}

TEST(result, equality_error_vs_value)
{
    zx::result<int, std::string> r1{ zx::error("failed") };
    zx::result<int, std::string> r2{ 42 };
    EXPECT_FALSE(r1 == r2);
    EXPECT_TRUE(r1 != r2);
}

TEST(result_ref, equality_both_references_same_value)
{
    int value1 = 42;
    int value2 = 42;
    zx::result<int&, std::string> r1{ value1 };
    zx::result<int&, std::string> r2{ value2 };
    EXPECT_TRUE(r1 == r2);
    EXPECT_FALSE(r1 != r2);
}

TEST(result_ref, equality_both_references_different_values)
{
    int value1 = 42;
    int value2 = 100;
    zx::result<int&, std::string> r1{ value1 };
    zx::result<int&, std::string> r2{ value2 };
    EXPECT_FALSE(r1 == r2);
    EXPECT_TRUE(r1 != r2);
}

TEST(result_ref, equality_both_errors_equal)
{
    zx::result<int&, std::string> r1{ zx::error("failed") };
    zx::result<int&, std::string> r2{ zx::error("failed") };
    EXPECT_TRUE(r1 == r2);
    EXPECT_FALSE(r1 != r2);
}

TEST(result_ref, equality_both_errors_not_equal)
{
    zx::result<int&, std::string> r1{ zx::error("failed") };
    zx::result<int&, std::string> r2{ zx::error("other error") };
    EXPECT_FALSE(r1 == r2);
    EXPECT_TRUE(r1 != r2);
}

TEST(result_ref, equality_reference_vs_error)
{
    int value = 42;
    zx::result<int&, std::string> r1{ value };
    zx::result<int&, std::string> r2{ zx::error("failed") };
    EXPECT_FALSE(r1 == r2);
    EXPECT_TRUE(r1 != r2);
}

TEST(result_ref, equality_error_vs_reference)
{
    int value = 42;
    zx::result<int&, std::string> r1{ zx::error("failed") };
    zx::result<int&, std::string> r2{ value };
    EXPECT_FALSE(r1 == r2);
    EXPECT_TRUE(r1 != r2);
}

TEST(result_void, equality_both_values)
{
    zx::result<void, std::string> r1;
    zx::result<void, std::string> r2;
    EXPECT_TRUE(r1 == r2);
    EXPECT_FALSE(r1 != r2);
}

TEST(result_void, equality_both_errors_equal)
{
    zx::result<void, std::string> r1{ zx::error("failed") };
    zx::result<void, std::string> r2{ zx::error("failed") };
    EXPECT_TRUE(r1 == r2);
    EXPECT_FALSE(r1 != r2);
}

TEST(result_void, equality_both_errors_not_equal)
{
    zx::result<void, std::string> r1{ zx::error("failed") };
    zx::result<void, std::string> r2{ zx::error("other error") };
    EXPECT_FALSE(r1 == r2);
    EXPECT_TRUE(r1 != r2);
}

TEST(result_void, equality_value_vs_error)
{
    zx::result<void, std::string> r1;
    zx::result<void, std::string> r2{ zx::error("failed") };
    EXPECT_FALSE(r1 == r2);
    EXPECT_TRUE(r1 != r2);
}

TEST(result_void, equality_error_vs_value)
{
    zx::result<void, std::string> r1{ zx::error("failed") };
    zx::result<void, std::string> r2;
    EXPECT_FALSE(r1 == r2);
    EXPECT_TRUE(r1 != r2);
}

TEST(result, try_invoke_success)
{
    auto func = []() { return 42; };
    auto result = zx::try_invoke(func);
    EXPECT_TRUE(result.has_value());
    EXPECT_THAT(*result, 42);
}

TEST(result, try_invoke_throws_exception)
{
    auto func = []() -> int { throw std::runtime_error("error"); };
    auto result = zx::try_invoke(func);
    EXPECT_TRUE(result.has_error());
    EXPECT_THAT(result.transform_error(get_error_message).error(), "error");
}

TEST(result, try_invoke_with_args)
{
    auto func = [](int a, int b) { return a + b; };
    auto result = zx::try_invoke(func, 10, 32);
    EXPECT_TRUE(result.has_value());
    EXPECT_THAT(*result, 42);
}

TEST(result, try_invoke_with_args_throws)
{
    auto func = [](int a, int b) -> int { throw std::runtime_error("addition failed"); };
    auto result = zx::try_invoke(func, 10, 32);
    EXPECT_TRUE(result.has_error());
    EXPECT_THAT(result.transform_error(get_error_message).error(), "addition failed");
}

TEST(result_void, try_invoke_success)
{
    int side_effect = 0;
    auto func = [&side_effect]() { side_effect = 42; };
    auto result = zx::try_invoke(func);
    EXPECT_TRUE(result.has_value());
    EXPECT_THAT(side_effect, 42);
}

TEST(result_void, try_invoke_throws_exception)
{
    auto func = []() { throw std::runtime_error("error"); };
    auto result = zx::try_invoke(func);
    EXPECT_TRUE(result.has_error());
    EXPECT_THAT(result.transform_error(get_error_message).error(), "error");
}

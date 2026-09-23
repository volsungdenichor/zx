#include <gmock/gmock.h>

#include <zx/type_name.hpp>

TEST(debug, type_name)
{
    EXPECT_THAT(zx::type_name<int>(), testing::Eq("int"));
    EXPECT_THAT(zx::type_name<std::string>(), testing::HasSubstr("basic_string"));
    EXPECT_THAT(zx::type_name<std::vector<int>>(), testing::HasSubstr("vector"));
}

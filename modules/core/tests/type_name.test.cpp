#include <gmock/gmock.h>

#include <zx/type_name.hpp>

TEST(debug, type_name)
{
    EXPECT_THAT(zx::type_name<int>(), testing::Eq("int"));
    EXPECT_THAT(zx::type_name<std::string>(), testing::Eq("std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >"));
    EXPECT_THAT(zx::type_name<std::vector<int>>(), testing::Eq("std::vector<int, std::allocator<int> >"));
}
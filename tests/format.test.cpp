#include <zx/zx.hpp>

#include "matchers.hpp"

TEST_CASE("format", "")
{
    REQUIRE(zx::str(123) == "123");
    REQUIRE(zx::str(123, 'x', 5) == "123x5");
    REQUIRE(zx::str(true) == "true");
    REQUIRE(zx::str(false) == "false");
    REQUIRE(zx::str(std::tuple{ 12, 'x', -5, true, "zx" }) == "(12, x, -5, true, zx)");
    REQUIRE(zx::str(std::pair{ 12, 'x' }) == "(12, x)");
    REQUIRE(zx::str(zx::delimit({ 1, 2, 3 }, ", ")) == "1, 2, 3");

#ifdef __GNUG__
    REQUIRE(zx::str(std::vector<int>{}) == "[std::vector<int, std::allocator<int> >]");
    try
    {
        throw std::runtime_error{ "some exception was thrown" };
    }
    catch (...)
    {
        REQUIRE(zx::str(std::current_exception()) == "std::exception_ptr<std::runtime_error>(some exception was thrown)");
    }
    try
    {
        throw "some exception was thrown";
    }
    catch (...)
    {
        REQUIRE(zx::str(std::current_exception()) == "std::exception_ptr<const char*>(some exception was thrown)");
    }
    try
    {
        throw 42;
    }
    catch (...)
    {
        REQUIRE(zx::str(std::current_exception()) == "std::exception_ptr<int>(42)");
    }
#endif
    REQUIRE(zx::str(zx::ostream_writer([](std::ostream& os) { os << "_" << 123 << "!"; })) == "_123!");
}

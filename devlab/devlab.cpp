#include <exception>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "zx/ansi/widgets/label.hpp"
#include "zx/ansi/widgets/layout.hpp"
#include "zx/app.hpp"
#include "zx/format.hpp"
#include "zx/functional.hpp"
#include "zx/maybe.hpp"
#include "zx/string.hpp"
#include "zx/widget.hpp"

void run(const std::vector<std::string_view>&)
{
    std::vector<std::pair<zx::string_t, zx::ansi::style_t>> list = {};

    const auto options = zx::create<zx::ansi::app_options>(
        [](zx::ansi::app_options& it)
        {
            it.use_alt_screen = true;
            it.hide_cursor = false;
            it.tick_ms = 100;
            it.mouse.button = true;
            it.mouse.drag = true;
            it.mouse.motion = true;
            it.mouse.sgr = true;
        });

    auto root = zx::ansi::widgets::hstack(
        zx::ansi::widgets::vstack(
            zx::ansi::widgets::label(zx::string_t{ "ALPHA" }), zx::ansi::widgets::label(zx::string_t{ "BETA" })),
        zx::ansi::widgets::vstack(
            zx::ansi::widgets::label(zx::string_t{ "GAMMA" }), zx::ansi::widgets::label(zx::string_t{ "DELTA" })));

    zx::ansi::app_t app{ root, options };
    app.run();
}

void handle_exception(std::exception_ptr ptr, int level = 0)
{
    if (!ptr)
    {
        return;
    }

    try
    {
        std::rethrow_exception(ptr);
    }
    catch (const std::exception& ex)
    {
        std::cerr << std::string(level * 2, ' ') << ex.what() << std::endl;
        try
        {
            std::rethrow_if_nested(ex);
        }
        catch (...)
        {
            handle_exception(std::current_exception(), level + 1);
        }
    }
    catch (const std::string& ex)
    {
        std::cerr << std::string(level * 2, ' ') << ex << std::endl;
    }
    catch (const char* ex)
    {
        std::cerr << std::string(level * 2, ' ') << ex << std::endl;
    }
    catch (...)
    {
        std::cerr << std::string(level * 2, ' ') << "unknown exception" << std::endl;
    }
}

int main(int argc, char* argv[])
{
    try
    {
        run(std::vector<std::string_view>(argv, argv + argc));
        return 0;
    }
    catch (...)
    {
        handle_exception(std::current_exception());
        return -1;
    }
}

#include <exception>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "zx/app.hpp"
#include "zx/format.hpp"
#include "zx/functional.hpp"
#include "zx/maybe.hpp"
#include "zx/string.hpp"
#include "zx/widget.hpp"

struct dummy_widget_t : public zx::ansi::widget_t::interface
{
    void render(zx::ansi::surface_t::mut_view_type dest) const override
    {
        for (auto& child : children())
        {
            child.render(dest);
        }
    }
    zx::ansi::surface_t::size_type preferred_size() const override { return { 1, 1 }; }
};

zx::ansi::widget_t dummy_widget()
{
    return zx::ansi::widget_t::make<dummy_widget_t>();
}

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

    zx::ansi::app_t app{
        [&](const zx::ansi::event_t& event)
        {
            if (auto e = std::get_if<zx::ansi::key_event_t>(&event))
            {
                list.emplace_back(zx::format(*e), zx::ansi::style_t{ zx::ansi::color_t::bright_cyan });
            }
            else if (auto e = std::get_if<zx::ansi::resize_event_t>(&event))
            {
                list.emplace_back(zx::format(*e), zx::ansi::style_t{ zx::ansi::color_t::bright_blue });
            }
            else if (auto e = std::get_if<zx::ansi::mouse_event_t>(&event))
            {
                list.emplace_back(zx::format(*e), zx::ansi::style_t{ zx::ansi::color_t::bright_red });
            }
            else if (auto e = std::get_if<zx::ansi::quit_event_t>(&event))
            {
                list.emplace_back(zx::format(*e), zx::ansi::style_t{ zx::ansi::color_t::blue });
            }

            while (list.size() > 20)
            {
                list.erase(list.begin());
            }
        },
        [&](zx::ansi::surface_t::mut_view_type view)
        {
            auto bounds = zx::ansi::surface_t::bounds_type::from_lower_size({ 0, 0 }, { 1, view.size()[1] });
            for (auto it = list.rbegin(); it != list.rend(); ++it)
            {
                if (!zx::mat::contains(view.bounds(), bounds))
                {
                    break;
                }
                const auto [text, style] = *it;
                zx::ansi::draw_text(view, bounds, text, style);
                bounds += zx::ansi::surface_t::location_type{ 1, 0 };
            }
        },
        options
    };
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

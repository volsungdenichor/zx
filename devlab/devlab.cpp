#include <chrono>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "zx/ansi/widgets/label.hpp"
#include "zx/ansi/widgets/layout.hpp"
#include "zx/app.hpp"
#include "zx/format.hpp"
#include "zx/functional.hpp"
#include "zx/image.hpp"
#include "zx/maybe.hpp"
#include "zx/string.hpp"
#include "zx/widget.hpp"

void run(const std::vector<std::string_view>&)
{
    using namespace zx;
    const auto background = mat::load_bitmap(mat::filepath_t{ "/home/krzysiek/river.bmp" });
    const auto conan = mat::load_bitmap(mat::filepath_t{ "/home/krzysiek/conan_small.bmp" });

    auto out = background;

    mat::modify(out, mat::lookup_table::contrast(1.F) * mat::lookup_table::brightness(20.F) * mat::lookup_table::negative());

    mat::copy(out.mut_view(), conan.view(), { 0, 0, 0 });
    mat::copy(out.mut_view(), mat::flip_horizontal(conan.view()), { 0, 200, 0 });
    mat::copy(out.mut_view(), mat::flip_vertical(conan.view()), { 0, 400, 0 });

    mat::copy(out.mut_view(), mat::rotate(conan.view(), 90), { 200, 0, 0 });
    mat::copy(out.mut_view(), mat::rotate(conan.view(), 180), { 200, 200, 0 });
    mat::copy(out.mut_view(), mat::rotate(conan.view(), 270), { 200, 400, 0 });

    mat::copy(out.mut_view(), mat::rotate(conan.view(), -90), { 400, 0, 0 });
    mat::copy(out.mut_view(), mat::rotate(conan.view(), -180), { 400, 200, 0 });
    mat::copy(out.mut_view(), mat::rotate(conan.view(), -270), { 400, 400, 0 });

    mat::draw_line(out, mat::segment(mat::point(200, -10), mat::point(300, 200)), mat::rgb_color_t{ 255, 255, 0 });

    mat::draw_circle(out, mat::circle(mat::point(300, 300), 100), mat::rgb_color_t{ 255, 0, 255 });

    mat::modify(
        out,
        mat::point(-1, 0),
        [](const mat::rgb_color_base_t<float>& color) -> mat::rgb_color_base_t<float> { return color * 10; });

    mat::save_bitmap(out, mat::filepath_t{ "/home/krzysiek/out.bmp" });
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
        const auto start = std::chrono::steady_clock::now();
        run(std::vector<std::string_view>(argv, argv + argc));
        const auto end = std::chrono::steady_clock::now();
        const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cerr << "Execution time: " << duration << " ms" << std::endl;
        return 0;
    }
    catch (...)
    {
        handle_exception(std::current_exception());
        return -1;
    }
}

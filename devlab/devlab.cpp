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
    const auto background = zx::mat::load_bitmap("/home/krzysiek/river.bmp");
    const auto conan = zx::mat::load_bitmap("/home/krzysiek/conan_small.bmp");

    auto out = background;

    zx::mat::at(out.mut_view(), { -1, -1 }, zx::mat::at(out.mut_view(), { -1, -1 }) * 3);

    zx::mat::copy(out.mut_view(), conan.view(), { 0, 0, 0 });
    zx::mat::copy(out.mut_view(), zx::mat::flip_horizontal(conan.view()), { 0, 200, 0 });
    zx::mat::copy(out.mut_view(), zx::mat::flip_vertical(conan.view()), { 0, 400, 0 });

    zx::mat::copy(out.mut_view(), zx::mat::rotate(conan.view(), 90), { 200, 0, 0 });
    zx::mat::copy(out.mut_view(), zx::mat::rotate(conan.view(), 180), { 200, 200, 0 });
    zx::mat::copy(out.mut_view(), zx::mat::rotate(conan.view(), 270), { 200, 400, 0 });

    zx::mat::copy(out.mut_view(), zx::mat::rotate(conan.view(), -90), { 400, 0, 0 });
    zx::mat::copy(out.mut_view(), zx::mat::rotate(conan.view(), -180), { 400, 200, 0 });
    zx::mat::copy(out.mut_view(), zx::mat::rotate(conan.view(), -270), { 400, 400, 0 });

    zx::mat::save_bitmap(out, "/home/krzysiek/out.bmp");
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

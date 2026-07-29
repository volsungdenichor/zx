#include <chrono>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include "zx/format.hpp"
#include "zx/functional.hpp"
#include "zx/image.hpp"
#include "zx/maybe.hpp"
#include "zx/raster.hpp"
#include "zx/string.hpp"

// cmake --build --preset ninja-release && ./build/ninja-release/devlab/zx_devlab && wslview ~/out.bmp
void run(const std::vector<std::string_view>&)
{
    using namespace zx;
    const auto background = mat::load_bitmap(mat::filepath_t{ "/home/krzysiek/river.bmp" });
    const auto conan = mat::load_bitmap(mat::filepath_t{ "/home/krzysiek/conan_small.bmp" });

    const auto temp = mat::with(
        background,
        [&](auto v)
        {
            mat::convolve(v, mat::kernel::median(mat::mask::square(9)));
            mat::convolve(v, mat::kernel::prewitt());
        });

    const auto shape = zx::mat::rasterize(
        mat::bounds(temp.slice({ { 0, -10 }, { 0, -10 }, {} })),
        [&](const mat::location_t<2>& loc)
        {
            const auto pixel = mat::filters::gray()(mat::at(temp, loc));
            return pixel[0] > 192.F;
        });

    const auto result = mat::with(
        background,
        [&](auto v)
        {
            mat::modify(v, mat::filters::sepia());
            mat::modify(v, mat::lookup_table::contrast(0.25F) * mat::lookup_table::brightness(-64.F));
            mat::draw_raster(v, shape, mat::true_color_t{ 0, 255, 0 });
            mat::paste(v, conan, mat::location_t<2>{ 600, 50 }, mat::filters::screen());
        });

    mat::save_bitmap(mat::flip_horizontal(result.view()), mat::filepath_t{ "/home/krzysiek/out.bmp" });
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

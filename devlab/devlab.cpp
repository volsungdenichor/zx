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
#include "zx/raster.hpp"
#include "zx/string.hpp"
#include "zx/widget.hpp"

struct rasterize_fn
{
    zx::mat::raster_t operator()(const zx::mat::rectangle_t<zx::mat::location_base_t>& shape) const
    {
        zx::mat::raster_t::shape_t raster_shape;

        for (zx::mat::location_base_t y = shape[0].get(zx::mat::side_t::lower); y < shape[0].get(zx::mat::side_t::upper);
             ++y)
        {
            raster_shape.emplace(
                y,
                std::vector<zx::mat::interval_type>{
                    { shape[1].get(zx::mat::side_t::lower), shape[1].get(zx::mat::side_t::upper) } });
        }

        return zx::mat::raster_t{ { raster_shape } };
    }

    zx::mat::raster_t operator()(const zx::mat::circle_t<zx::mat::location_base_t>& shape) const
    {
        zx::mat::raster_t::shape_t raster_shape;
        const auto center = shape.center;

        auto output_row = [&](zx::mat::location_base_t y, zx::mat::interval_type interval)
        {
            auto [it, inserted] = raster_shape.emplace(y, std::vector<zx::mat::interval_type>{});
            if (inserted || it->second.empty())
            {
                it->second.push_back(interval);
                return;
            }

            auto& span = it->second.front();
            span[0] = std::min(span[0], interval[0]);
            span[1] = std::max(span[1], interval[1]);
        };

        zx::mat::vector_t<2, zx::mat::location_base_t> cur{ shape.radius, 0 };
        int err = 0;

        while (cur[0] >= cur[1])
        {
            output_row(center[0] + cur[1], { center[1] - cur[0], center[1] + cur[0] + 1 });
            output_row(center[0] - cur[1], { center[1] - cur[0], center[1] + cur[0] + 1 });
            output_row(center[0] + cur[0], { center[1] - cur[1], center[1] + cur[1] + 1 });
            output_row(center[0] - cur[0], { center[1] - cur[1], center[1] + cur[1] + 1 });

            if (err <= 0)
            {
                cur[1] += 1;
                err += 2 * cur[1] + 1;
            }

            if (err > 0)
            {
                cur[0] -= 1;
                err -= 2 * cur[0] + 1;
            }
        }
        return zx::mat::raster_t{ { raster_shape } };
    }

    zx::mat::raster_t operator()(
        const zx::mat::rectangle_t<zx::mat::location_base_t>& area,
        zx::function_ref<bool(const zx::mat::location_t<2>&)> predicate) const
    {
        zx::mat::raster_t::shape_t raster_shape;

        const auto output_interval
            = [&](zx::mat::location_base_t y, zx::mat::interval_type interval) { raster_shape[y].push_back(interval); };

        for (zx::mat::location_base_t y = area[0].get(zx::mat::side_t::lower); y < area[0].get(zx::mat::side_t::upper); ++y)
        {
            bool in_interval = false;
            zx::mat::location_base_t interval_start = 0;

            for (zx::mat::location_base_t x = area[1].get(zx::mat::side_t::lower); x < area[1].get(zx::mat::side_t::upper);
                 ++x)
            {
                const zx::mat::location_t<2> loc{ y, x };
                if (predicate(loc))
                {
                    if (!in_interval)
                    {
                        interval_start = x;
                        in_interval = true;
                    }
                }
                else if (in_interval)
                {
                    output_interval(y, { interval_start, x });
                    in_interval = false;
                }
            }

            if (in_interval)
            {
                output_interval(y, { interval_start, area[1].get(zx::mat::side_t::upper) });
            }
        }

        return zx::mat::raster_t{ { raster_shape } };
    }
};

static constexpr inline auto rasterize = rasterize_fn{};

// cmake --build --preset ninja-release && ./build/ninja-release/devlab/zx_devlab && wslview ~/out.bmp
void run(const std::vector<std::string_view>&)
{
    using namespace zx;
    const auto background = mat::load_bitmap(mat::filepath_t{ "/home/krzysiek/river.bmp" });
    const auto conan = mat::load_bitmap(mat::filepath_t{ "/home/krzysiek/conan_small.bmp" });

    const auto temp = mat::with(
        background,
        [](auto v) { mat::convolve(v, mat::kernel::median(mat::mask::square(3))); },
        [](auto v) { mat::convolve(v, mat::kernel::sobel()); });

    const auto shape = rasterize(
        mat::bounds(temp.slice({ { 0, -4 }, { 0, -4 }, {} })),
        [&](const mat::location_t<2>& loc)
        {
            const auto pixel = mat::filters::gray()(mat::at(temp, loc));
            return pixel[0] > 128.F;
        });

    auto result = mat::with(
        background,
        [](auto v) { mat::modify(v, mat::filters::sepia()); },
        [](auto v) { mat::modify(v, mat::lookup_table::contrast(0.25F) * mat::lookup_table::brightness(-64.F)); },
        [&](auto v) {
            mat::draw_raster(v, shape, mat::true_color_t{ 0, 255, 0 });
        },
        [&](auto v) {
            mat::paste(v, conan, mat::location_t<2>{ 600, 50 }, mat::filters::screen());
        });

    mat::save_bitmap(result, mat::filepath_t{ "/home/krzysiek/out.bmp" });
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

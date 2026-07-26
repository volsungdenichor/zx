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

struct sepia
{
    zx::mat::rgb_color_float_t operator()(const zx::mat::rgb_color_float_t& color) const
    {
        static const std::array<std::array<float, 3>, 3> coeffs
            = { { { 0.393F, 0.769F, 0.189F }, { 0.349F, 0.686F, 0.168F }, { 0.272F, 0.534F, 0.131F } } };

        zx::mat::rgb_color_float_t result;
        for (std::size_t i = 0; i < 3; ++i)
        {
            result[i] = std::inner_product(coeffs[i].begin(), coeffs[i].end(), color.begin(), 0.F);
        }
        return result;
    }
};

struct gray
{
    zx::mat::rgb_color_float_t operator()(const zx::mat::rgb_color_float_t& color) const
    {
        static const std::array<float, 3> coeffs = { 0.299F, 0.587F, 0.114F };

        const float gray_value = std::inner_product(coeffs.begin(), coeffs.end(), color.begin(), 0.F);
        return zx::mat::rgb_color_float_t{ gray_value, gray_value, gray_value };
    }
};

zx::mat::raster_t rasterize(const zx::mat::rectangle_t<zx::mat::location_base_t>& shape)
{
    zx::mat::raster_t::shape_t raster_shape;

    for (zx::mat::location_base_t y = shape[0].get(zx::mat::side_t::lower); y < shape[0].get(zx::mat::side_t::upper); ++y)
    {
        raster_shape.emplace(
            y,
            std::vector<zx::mat::interval_type>{
                { shape[1].get(zx::mat::side_t::lower), shape[1].get(zx::mat::side_t::upper) } });
    }

    return zx::mat::raster_t{ { raster_shape } };
}

zx::mat::raster_t rasterize(const zx::mat::circle_t<zx::mat::location_base_t>& shape)
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

zx::mat::raster_t rasterize(
    const zx::mat::rectangle_t<zx::mat::location_base_t>& area,
    zx::function_ref<bool(const zx::mat::location_t<2>&)> predicate)
{
    zx::mat::raster_t::shape_t raster_shape;

    const auto output_interval
        = [&](zx::mat::location_base_t y, zx::mat::interval_type interval) { raster_shape[y].push_back(interval); };

    for (zx::mat::location_base_t y = area[0].get(zx::mat::side_t::lower); y < area[0].get(zx::mat::side_t::upper); ++y)
    {
        bool in_interval = false;
        zx::mat::location_base_t interval_start = 0;

        for (zx::mat::location_base_t x = area[1].get(zx::mat::side_t::lower); x < area[1].get(zx::mat::side_t::upper); ++x)
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

// cmake --build --preset ninja-release && ./build/ninja-release/devlab/zx_devlab && wslview ~/out.bmp
void run(const std::vector<std::string_view>&)
{
    using namespace zx;
    const auto background = mat::load_bitmap(mat::filepath_t{ "/home/krzysiek/river.bmp" });
    const auto conan = mat::load_bitmap(mat::filepath_t{ "/home/krzysiek/conan_small.bmp" });

    auto out = background;

    // mat::modify(out, mat::lookup_table::contrast(1.F) * mat::lookup_table::brightness(20.F));
    mat::modify(out, sepia{});

    mat::copy(out.mut_view(), conan.view(), { 0, 0, 0 });
    mat::copy(out.mut_view(), mat::flip_horizontal(conan.view()), { 0, 200, 0 });
    mat::copy(out.mut_view(), mat::flip_vertical(conan.view()), { 0, 400, 0 });

    mat::copy(out.mut_view(), mat::rotate(conan.view(), 90), { 200, 0, 0 });
    mat::copy(out.mut_view(), mat::rotate(conan.view(), 180), { 200, 200, 0 });
    mat::copy(out.mut_view(), mat::rotate(conan.view(), 270), { 200, 400, 0 });

    mat::copy(out.mut_view(), mat::rotate(conan.view(), -90), { 400, 0, 0 });
    mat::copy(out.mut_view(), mat::rotate(conan.view(), -180), { 400, 200, 0 });
    mat::copy(out.mut_view(), mat::rotate(conan.view(), -270), { 400, 400, 0 });

    const auto outer = rasterize({ mat::point(300, 300), 100 });
    const auto inner = rasterize({ mat::point(300, 450), 100 });
    const auto rect = rasterize(mat::box::from_center_extent(mat::point(300, 500), mat::extent(100, 200)));

    const auto stripes = rasterize(
        mat::box::from_center_extent(mat::point(300, 300), mat::extent(220, 220)),
        [&](const mat::location_t<2>& loc) { return loc[1] % 5 == 0; });

    // mat::draw_raster(out.mut_view(), outer, zx::mat::rgb_color_t{ 255, 255, 0 });
    // mat::draw_raster(out.mut_view(), inner, zx::mat::rgb_color_t{ 255, 0, 255 });
    // mat::draw_raster(out.mut_view(), rect, zx::mat::rgb_color_t{ 255, 255, 255 });
    mat::draw_raster(out.mut_view(), outer - stripes, zx::mat::rgb_color_t{ 255, 100, 32 });

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

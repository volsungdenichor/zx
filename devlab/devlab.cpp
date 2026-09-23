#include <algorithm>
#include <chrono>
#include <cmath>
#include <exception>
#include <iostream>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

#include "perlin.hpp"
#include "poisson.hpp"
#include "zx/format.hpp"
#include "zx/functional.hpp"
#include "zx/image.hpp"
#include "zx/let.hpp"
#include "zx/maybe.hpp"
#include "zx/random.hpp"
#include "zx/raster.hpp"
#include "zx/string.hpp"
#include "zx/yield.hpp"

template <class In, class Out>
struct interpolate_fn
{
    std::pair<In, In> m_in;
    std::pair<Out, Out> m_out;

    auto operator()(In value) const -> Out
    {
        return m_out.first + (m_out.second - m_out.first) * (value - m_in.first) / (m_in.second - m_in.first);
    }
};

template <class Clock = std::chrono::high_resolution_clock, class Func, class... Args>
auto time_it(Func&& func, Args&&... args) -> std::pair<std::invoke_result_t<Func, Args...>, typename Clock::duration>
{
    const auto start = Clock::now();
    const auto result = std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
    const auto end = Clock::now();
    return { result, end - start };
}

template <class Clock = std::chrono::high_resolution_clock, class Func, class... Args>
auto try_time_it(Func&& func, Args&&... args)
    -> zx::result_t<std::pair<std::invoke_result_t<Func, Args...>, typename Clock::duration>, std::exception_ptr>
{
    auto [result, duration] = time_it<Clock>(zx::try_invoke, std::forward<Func>(func), std::forward<Args>(args)...);
    if (!result)
    {
        return zx::forward_error(std::move(result));
    }
    return std::pair{ *std::move(result), duration };
}

/*
cmake --preset ninja-release -DZX_BUILD_DEVLAB=ON
cmake --build --preset ninja-release --target zx_devlab
./build/ninja-release/devlab/zx_devlab
*/
int run(const std::vector<std::string_view>&)
{
    using namespace zx;

    for (const auto p : poisson(mat::box::from_lower_upper(mat::point(0.F, 0.F), mat::point(100.F, 100.F)), 5.F, 30))
    {
        std::cout << p << std::endl;
    }

    const auto permutations = std::invoke(
        []() -> std::vector<int>
        {
            std::vector<int> result(256);
            std::iota(result.begin(), result.end(), 0);
            std::shuffle(result.begin(), result.end(), std::mt19937{ std::random_device{}() });
            return result;
        });

    const auto get_permutation = [&](int index) -> int { return permutations[index % permutations.size()]; };

    const auto background = mat::load_bitmap(mat::filepath_t{ "/home/krzysiek/river.bmp" });
    const auto conan = mat::load_bitmap(mat::filepath_t{ "/home/krzysiek/conan_small.bmp" });

    const auto create_perlin = [&](const mat::array_t<float, 2>::extent_type& extent) -> mat::rgb_image_t
    {
        mat::array_t<float, 2> result(extent);
        mat::detail::for_each(
            result.shape(), [&](const mat::location_t<2>& loc) { result[loc] = perlin(loc / 20.F, get_permutation); });
        const auto normalize = interpolate_fn<float, float>{ zx::from(result) | zx::min_max_value<float>(), { 0.F, 255.F } };
        zx::from(result) | zx::transform(normalize) | zx::copy_to(result.begin());
        mat::rgb_image_t res(extent);
        mat::detail::for_each(
            res.data().shape(),
            [&](const mat::rgb_image_t::location_type& loc) {
                res[loc] = mat::rgb_color_t{ result[loc], result[loc], result[loc] };
            });
        return res;
    };

    // const auto perlin = create_perlin(background.extent());

    const auto temp = mat::with(
        background,
        [&](auto v)
        {
            mat::convolve(v, mat::kernel::median(mat::mask::square(9)));
            mat::convolve(v, mat::kernel::prewitt());
        });

    const auto shape = zx::mat::rasterize(
        temp.slice({ { 0, -10 }, { 0, -10 } }).bounds(),
        [&](const mat::rgb_image_t::location_type& loc)
        {
            const auto pixel = mat::filters::gray(temp[loc]);
            return pixel[0] > 192.F;
        });

    const auto result = mat::with(
        background,
        [&](auto v)
        {
            mat::modify(v, mat::filters::sepia);
            mat::modify(v, mat::lookup_table::contrast(0.25F) * mat::lookup_table::brightness(-64.F));
            mat::draw_raster(v, shape, mat::filters::solid(mat::true_color_t{ 255, 0, 0 }));
            mat::paste(v, conan, mat::rgb_image_t::location_type{ 600, 50 }, mat::filters::blend(0.5F));
            // mat::paste(v, perlin, mat::rgb_image_t::location_type{ 0, 0 }, mat::filters::blend(0.125F));
        });

    mat::save_bitmap(mat::flip_horizontal(result.view()), mat::filepath_t{ "/home/krzysiek/out.bmp" });
    return 0;
}

struct exception_handler_t
{
    std::function<void(int, std::string_view)> handler;

    void operator()(std::exception_ptr ptr, int level = 0) const
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
            handler(level, ex.what());
            try
            {
                std::rethrow_if_nested(ex);
            }
            catch (...)
            {
                (*this)(std::current_exception(), level + 1);
            }
        }
        catch (const std::string& ex)
        {
            handler(level, ex);
        }
        catch (const char* ex)
        {
            handler(level, ex);
        }
        catch (...)
        {
            handler(level, "unknown exception");
        }
    }
};

int main(int argc, char* argv[])
{
    const auto handle_exception = exception_handler_t{ [](int level, std::string_view message)
                                                       { zx::format_to(std::cerr, std::string(level * 2, ' '), message); } };

    if (auto res = try_time_it(run, std::vector<std::string_view>(argv, argv + argc)))
    {
        const auto [result, duration] = *res;
        std::cerr << "Execution time: " << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() << " ms"
                  << std::endl;
        return result;
    }
    else
    {
        handle_exception(res.error());
        return -1;
    }
}

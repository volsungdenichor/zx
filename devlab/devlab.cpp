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

struct min_max_fn
{
    template <class Iter, class T = zx::iter_value_t<Iter>>
    auto operator()(Iter begin, Iter end) const -> std::pair<T, T>
    {
        if (begin == end)
        {
            throw std::invalid_argument{ "Range cannot be empty" };
        }
        T min_value = *begin;
        T max_value = min_value;
        for (++begin; begin != end; ++begin)
        {
            min_value = std::min(min_value, *begin);
            max_value = std::max(max_value, *begin);
        }
        return { min_value, max_value };
    }

    template <class Range>
    auto operator()(Range&& range) const
    {
        return (*this)(std::begin(range), std::end(range));
    }
};

static constexpr inline auto min_max = min_max_fn{};

/*
cmake --preset ninja-release -DZX_BUILD_DEVLAB=ON
cmake --build --preset ninja-release --target zx_devlab
./build/ninja-release/devlab/zx_devlab
*/
void run(const std::vector<std::string_view>&)
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
        const auto normalize = interpolate_fn<float, float>{ min_max(result), { 0.F, 255.F } };
        std::transform(std::begin(result), std::end(result), std::begin(result), normalize);
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

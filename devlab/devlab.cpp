#include <chrono>
#include <exception>
#include <iostream>
#include <random>
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

struct perlin_fn
{
    template <std::size_t D, class T, std::enable_if_t<(D > 0), int> = 0>
    T operator()(const zx::mat::vector_t<D, T>& loc, zx::function_ref<int(int)> get_permutation) const
    {
        static const auto corners = create_product<D>();

        const auto [floor_values, rel_values] = zx::mat::floor_and_fractional_part(loc);

        std::array<T, D> faded{};
        for (std::size_t d = 0; d < D; ++d)
        {
            faded[d] = fade(rel_values[d]);
        }

        const auto corner_value = [&](const zx::mat::vector_t<D, int>& corner) -> T
        {
            const zx::mat::vector_t<D, T> coords = floor_values + corner;

            int hash_value = 0;
            for (std::size_t d = 0; d < D; ++d)
            {
                hash_value = get_permutation(hash_value + static_cast<int>(coords[d]) + static_cast<int>(d));
            }

            return grad(hash_value, rel_values - corner);
        };

        std::array<T, static_cast<std::size_t>(1) << D> values{};
        for (std::size_t i = 0; i < corners.size(); ++i)
        {
            values[i] = corner_value(corners[i]);
        }

        return interpolate<D>(faded, values);
    }

    template <std::size_t N>
    using pow2 = std::integral_constant<std::size_t, static_cast<std::size_t>(1) << N>;

    template <std::size_t D>
    static constexpr auto create_product() -> std::array<zx::mat::vector_t<D, int>, pow2<D>::value>
    {
        if constexpr (D == 1)
        {
            return { zx::mat::vector_t<1, int>{ 0 }, zx::mat::vector_t<1, int>{ 1 } };
        }
        else
        {
            std::array<zx::mat::vector_t<D, int>, pow2<D>::value> result{};
            const auto sub_product = create_product<D - 1>();
            for (int i = 0; i < 2; ++i)
            {
                for (std::size_t j = 0; j < sub_product.size(); ++j)
                {
                    result[i * sub_product.size() + j] = zx::mat::prepend(sub_product[j], i);
                }
            }
            return result;
        }
    }

    template <class T>
    static constexpr T fade(T t)
    {
        return t * t * t * (t * (t * 6 - 15) + 10);
    }

    template <class R, class T>
    static constexpr auto lerp(R ratio, T a, T b) -> T
    {
        return a + ratio * (b - a);
    }

    template <std::size_t D, class T>
    static constexpr T grad(int hash, const std::array<T, D>& vector)
    {
        const auto sign = [](bool is_negative, T value) -> T { return is_negative ? -value : value; };

        if constexpr (D == 1)
        {
            return sign((hash & 1) != 0, vector[0]);
        }
        else
        {
            const std::size_t first_axis = static_cast<std::size_t>(hash) % D;
            const std::size_t second_axis = (first_axis + 1 + (static_cast<std::size_t>(hash >> 2) % (D - 1))) % D;

            return sign((hash & 1) != 0, vector[first_axis]) + sign((hash & 2) != 0, vector[second_axis]);
        }
    }

    template <std::size_t D, class T, std::size_t D2>
    static constexpr T interpolate(const std::array<T, D>& faded, const std::array<T, D2>& values)
    {
        if constexpr (D == 0)
        {
            return values[0];
        }
        else
        {
            std::array<T, (D2 / 2)> next{};
            for (std::size_t i = 0; i < next.size(); ++i)
            {
                next[i] = lerp(faded[D - 1], values[2 * i + 0], values[2 * i + 1]);
            }

            std::array<T, D - 1> remaining_faded{};
            for (std::size_t i = 0; i < D - 1; ++i)
            {
                remaining_faded[i] = faded[i];
            }

            return interpolate<D - 1, T, (D2 / 2)>(remaining_faded, next);
        }
    }
};

static constexpr inline perlin_fn perlin = {};

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

// cmake --build --preset ninja-release && ./build/ninja-release/devlab/zx_devlab && wslview ~/out.bmp
void run(const std::vector<std::string_view>&)
{
    const auto permutations = std::invoke(
        []() -> std::vector<int>
        {
            std::vector<int> result(256);
            std::iota(result.begin(), result.end(), 0);
            std::shuffle(result.begin(), result.end(), std::mt19937{ std::random_device{}() });
            return result;
        });

    const auto get_permutation = [&](int index) -> int { return permutations[index % permutations.size()]; };

    using namespace zx;
    const auto background = mat::load_bitmap(mat::filepath_t{ "/home/krzysiek/river.bmp" });
    const auto conan = mat::load_bitmap(mat::filepath_t{ "/home/krzysiek/conan_small.bmp" });

    const auto create_perlin = [&](const mat::array_t<float, 2>::extent_type& extent) -> mat::rgb_image_t
    {
        mat::array_t<float, 2> result(extent);
        mat::detail::for_each(
            result.shape(),
            [&](const mat::location_t<2>& loc) { result[loc] = perlin(loc.to<float>() / 20.F, get_permutation); });
        float min_value = std::numeric_limits<float>::max();
        float max_value = std::numeric_limits<float>::lowest();
        for (auto v : result)
        {
            min_value = std::min(min_value, v);
            max_value = std::max(max_value, v);
        }
        const auto f = interpolate_fn<float, float>{ { min_value, max_value }, { 0.F, 255.F } };
        for (auto& v : result)
        {
            v = f(v);
        }
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

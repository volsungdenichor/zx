#pragma once

#include "zx/array.hpp"
#include "zx/function_ref.hpp"

struct perlin_fn
{
    template <std::size_t N>
    using pow2 = std::integral_constant<std::size_t, static_cast<std::size_t>(1) << N>;

    template <std::size_t D>
    using corner_t = zx::mat::vector_t<D, int>;

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

        const auto corner_value = [&](const corner_t<D>& corner) -> T
        {
            const zx::mat::vector_t<D, T> coords = floor_values + corner;

            int hash_value = 0;
            for (std::size_t d = 0; d < D; ++d)
            {
                hash_value = get_permutation(hash_value + static_cast<int>(coords[d]) + static_cast<int>(d));
            }

            return grad(hash_value, rel_values - corner);
        };

        std::array<T, pow2<D>::value> values{};
        for (std::size_t i = 0; i < corners.size(); ++i)
        {
            values[i] = corner_value(corners[i]);
        }

        return interpolate(faded, values);
    }

    template <std::size_t D>
    static constexpr auto create_product() -> std::array<corner_t<D>, pow2<D>::value>
    {
        if constexpr (D == 1)
        {
            return { corner_t<1>{ 0 }, corner_t<1>{ 1 } };
        }
        else
        {
            std::array<corner_t<D>, pow2<D>::value> result{};
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

    template <class R>
    static constexpr auto lerp(R ratio)
    {
        return [=](auto a, auto b) { return a + ratio * (b - a); };
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

    template <std::size_t D, std::size_t D2, class T>
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
                next[i] = lerp(faded[D - 1])(values[2 * i + 0], values[2 * i + 1]);
            }

            std::array<T, D - 1> remaining_faded{};
            for (std::size_t i = 0; i < D - 1; ++i)
            {
                remaining_faded[i] = faded[i];
            }

            return interpolate<D - 1, (D2 / 2), T>(remaining_faded, next);
        }
    }
};

static constexpr inline auto perlin = perlin_fn{};
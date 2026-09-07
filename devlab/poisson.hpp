#pragma once

#include <functional>

#include "zx/array.hpp"
#include "zx/let.hpp"
#include "zx/maybe.hpp"
#include "zx/random.hpp"
#include "zx/yield.hpp"

template <class T>
using supplier_t = std::function<T()>;

template <class Func, class Pred>
auto repeat_until(Func func, Pred pred, std::size_t max_attempts) -> zx::maybe_t<std::invoke_result_t<Func>>
{
    for (std::size_t attempt = 0; attempt < max_attempts; ++attempt)
    {
        const auto result = func();
        if (pred(result))
        {
            return result;
        }
    }
    return zx::none;
}

template <std::size_t D, class T>
supplier_t<zx::mat::vector_t<D, T>> random_point(const zx::mat::box_shape_t<D, T>& bounds, zx::random::seed_t seed = {})
{
    std::array<supplier_t<T>, D> generators;
    for (std::size_t d = 0; d < D; ++d)
    {
        generators[d] = zx::random::uniform(bounds[d][0], bounds[d][1], seed);
    }

    return [generators]() mutable
    {
        zx::mat::vector_t<D, T> point;
        for (std::size_t d = 0; d < D; ++d)
        {
            point[d] = generators[d]();
        }
        return point;
    };
}

template <std::size_t D, class T>
supplier_t<zx::mat::vector_t<D, T>> random_point(T radius, zx::random::seed_t seed = {})
{
    const auto direction_generator = zx::random::normal(T{}, T{ 1 }, seed);
    const auto volume_generator = zx::random::uniform(
        static_cast<T>(std::pow(radius, static_cast<T>(D))), static_cast<T>(std::pow(2 * radius, static_cast<T>(D))), seed);

    return [direction_generator, volume_generator]() -> zx::mat::vector_t<D, T>
    {
        zx::mat::vector_t<D, T> direction;
        do
        {
            for (std::size_t d = 0; d < D; ++d)
            {
                direction[d] = direction_generator();
            }
        } while (zx::mat::norm(direction) == T{});

        const auto distance = static_cast<T>(std::pow(volume_generator(), T{ 1 } / static_cast<T>(D)));
        return direction * (distance / zx::mat::length(direction));
    };
}

using sample_t = int;

template <std::size_t D, class T>
zx::mat::array_t<sample_t, D> prepare_grid(const zx::mat::extent_t<D, T>& bounds, T cell_size)
{
    typename zx::mat::array_t<sample_t, D>::extent_type grid_extent;
    for (std::size_t d = 0; d < D; ++d)
    {
        grid_extent[d] = static_cast<zx::mat::extent_base_t>(zx::mat::math::ceil(bounds[d] / cell_size));
    }
    return zx::mat::array_t<sample_t, D>{ grid_extent, sample_t{ -1 } };
}

struct poisson_fn
{
    template <std::size_t D, class T, zx::enable_if_t<(D > 0)> = 0>
    std::vector<zx::mat::vector_t<D, T>> operator()(
        const zx::mat::box_shape_t<D, T>& bounds, T radius, std::size_t k, zx::random::seed_t seed = {}) const
    {
        using sample_t = int;
        using grid_t = zx::mat::array_t<sample_t, D>;

        if (radius <= T{})
        {
            throw std::invalid_argument{ "Poisson radius must be positive" };
        }

        const auto cell_size = radius / zx::mat::math::sqrt(static_cast<T>(D));

        const auto search_radius = static_cast<zx::mat::location_base_t>(zx::mat::math::ceil(radius / cell_size));
        const auto offset = grid_t::location_type::ones() * search_radius;

        const auto pos_generator = random_point(bounds, seed);
        const auto point_generator = random_point<D, T>(radius, seed);

        const auto get_grid_location = [&](const zx::mat::point_t<D, T>& location) -> typename grid_t::location_type
        { return zx::mat::floor((location - zx::mat::lower(bounds)) / cell_size).template to<zx::mat::location_base_t>(); };

        const auto make_surrounding = [&](const zx::mat::point_t<D, T>& point) {
            return zx::mat::spherical_shape_t<D, T>{ point, radius };
        };

        static const auto surrounding_contains = [](const zx::mat::point_t<D, T>& candidate)
        {
            return [=](const zx::mat::spherical_shape_t<D, T>& spherical_shape)
            { return zx::mat::contains(spherical_shape, candidate); };
        };

        static const auto is_valid_sample = [](sample_t sample) { return sample != sample_t{ -1 }; };

        grid_t grid = prepare_grid(zx::mat::size(bounds), cell_size);
        std::vector<zx::mat::point_t<D, T>> points;
        std::vector<sample_t> active_samples;

        const auto add_point = [&](const zx::mat::point_t<D, T>& point)
        {
            const auto sample = static_cast<sample_t>(points.size());
            points.push_back(point);
            active_samples.push_back(sample);
            grid[get_grid_location(point)] = sample;
        };

        const auto get_point_from_sample
            = [&](sample_t sample) -> const zx::mat::point_t<D, T>& { return points[static_cast<std::size_t>(sample)]; };

        const auto is_valid = [&](const zx::mat::point_t<D, T>& candidate)
        {
            return zx::mat::contains(bounds, candidate)
                   && (zx::from(grid.region(zx::mat::box::from_center_radius(get_grid_location(candidate), offset)))  //
                       | zx::filter(is_valid_sample)                                                                  //
                       | zx::transform(get_point_from_sample)                                                         //
                       | zx::transform(make_surrounding)                                                              //
                       | zx::none_of(surrounding_contains(candidate)));
        };

        add_point(pos_generator());

        while (!active_samples.empty())
        {
            const auto active_index = zx::random::uniform(std::size_t{ 0 }, active_samples.size() - 1, seed)();

            const auto found = zx::let(
                get_point_from_sample(active_samples[active_index]),
                [&](const zx::mat::point_t<D, T>& center) -> zx::maybe_t<zx::mat::point_t<D, T>>
                { return repeat_until([&]() { return center + point_generator(); }, is_valid, k); });

            if (found)
            {
                add_point(*found);
            }
            else
            {
                active_samples.erase(active_samples.begin() + active_index);
            }
        }

        return points;
    }
};

constexpr inline auto poisson = poisson_fn{};

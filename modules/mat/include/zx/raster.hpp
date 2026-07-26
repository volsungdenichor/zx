#pragma once

#include <algorithm>
#include <cstddef>
#include <map>
#include <numeric>
#include <vector>
#include <zx/array.hpp>

namespace zx
{
namespace mat
{

struct raster_t
{
    using shape_t = std::map<location_base_t, std::vector<interval_type>>;

    using shape_list_t = std::vector<shape_t>;
    using const_iterator = shape_list_t::const_iterator;
    using size_type = shape_list_t::size_type;

    shape_list_t m_shapes;

    raster_t() = default;
    raster_t(shape_list_t shapes) : m_shapes(normalize_disjoint(flatten(shapes))) { }

    const_iterator begin() const { return m_shapes.begin(); }
    const_iterator end() const { return m_shapes.end(); }
    size_type size() const { return m_shapes.size(); }

    friend raster_t operator+(const raster_t& lhs, const raster_t& rhs)
    {
        return raster_t{ normalize_disjoint(unite(flatten(lhs.m_shapes), flatten(rhs.m_shapes))) };
    }

    friend raster_t operator*(const raster_t& lhs, const raster_t& rhs)
    {
        return raster_t{ normalize_disjoint(intersection(flatten(lhs.m_shapes), flatten(rhs.m_shapes))) };
    }

    friend raster_t operator-(const raster_t& lhs, const raster_t& rhs)
    {
        return raster_t{ normalize_disjoint(difference(flatten(lhs.m_shapes), flatten(rhs.m_shapes))) };
    }

    static raster_t dilate(const raster_t& raster, location_base_t radius)
    {
        return raster_t{ normalize_disjoint(dilate(flatten(raster.m_shapes), radius)) };
    }

    static raster_t erode(const raster_t& raster, location_base_t radius)
    {
        return raster_t{ normalize_disjoint(erode(flatten(raster.m_shapes), radius)) };
    }

    static raster_t outline(const raster_t& raster, location_base_t radius) { return dilate(raster, radius) - raster; }

private:
    static shape_t normalize(const shape_t& shape)
    {
        shape_t result;
        for (const auto& [y, span] : shape)
        {
            if (span.empty())
            {
                continue;
            }
            std::vector<interval_type> v = span;
            std::sort(
                v.begin(),
                v.end(),
                [](const interval_type& a, const interval_type& b) { return (a[0] < b[0]) || (a[0] == b[0] && a[1] < b[1]); });
            std::vector<interval_type> merged = { v[0] };
            for (std::size_t i = 1; i < v.size(); ++i)
            {
                auto& last = merged.back();
                if (v[i][0] <= last[1])
                {
                    last[1] = std::max(last[1], v[i][1]);
                }
                else
                {
                    merged.push_back(v[i]);
                }
            }
            result[y] = std::move(merged);
        }
        return result;
    }

    static shape_t flatten(const shape_list_t& shapes)
    {
        shape_t result;
        for (const auto& shape : shapes)
        {
            result = unite(result, shape);
        }
        return result;
    }

    static shape_t unite(const shape_t& lhs, const shape_t& rhs)
    {
        shape_t result = lhs;
        for (const auto& [y, rhs_spans] : rhs)
        {
            auto& spans = result[y];
            spans.insert(spans.end(), rhs_spans.begin(), rhs_spans.end());
        }
        return normalize(result);
    }

    static shape_t intersection(const shape_t& lhs, const shape_t& rhs)
    {
        const shape_t lhs_norm = normalize(lhs);
        const shape_t rhs_norm = normalize(rhs);

        shape_t result;
        for (const auto& [y, lhs_spans] : lhs_norm)
        {
            const auto it = rhs_norm.find(y);
            if (it == rhs_norm.end())
            {
                continue;
            }

            const auto& rhs_spans = it->second;
            std::size_t i = 0;
            std::size_t j = 0;
            std::vector<interval_type> row;
            while (i < lhs_spans.size() && j < rhs_spans.size())
            {
                const auto lo = std::max(lhs_spans[i][0], rhs_spans[j][0]);
                const auto hi = std::min(lhs_spans[i][1], rhs_spans[j][1]);
                if (lo < hi)
                {
                    row.push_back({ lo, hi });
                }

                if (lhs_spans[i][1] < rhs_spans[j][1])
                {
                    ++i;
                }
                else
                {
                    ++j;
                }
            }

            if (!row.empty())
            {
                result[y] = std::move(row);
            }
        }

        return result;
    }

    static shape_t difference(const shape_t& lhs, const shape_t& rhs)
    {
        const shape_t lhs_norm = normalize(lhs);
        const shape_t rhs_norm = normalize(rhs);

        shape_t result;
        for (const auto& [y, lhs_spans] : lhs_norm)
        {
            const auto it = rhs_norm.find(y);
            if (it == rhs_norm.end())
            {
                result[y] = lhs_spans;
                continue;
            }

            const auto& rhs_spans = it->second;
            std::vector<interval_type> row;
            std::size_t j = 0;
            for (const auto& span : lhs_spans)
            {
                location_base_t cursor = span[0];
                while (j < rhs_spans.size() && rhs_spans[j][1] <= cursor)
                {
                    ++j;
                }

                std::size_t k = j;
                while (k < rhs_spans.size() && rhs_spans[k][0] < span[1])
                {
                    if (rhs_spans[k][0] > cursor)
                    {
                        row.push_back({ cursor, std::min(span[1], rhs_spans[k][0]) });
                    }
                    cursor = std::max(cursor, rhs_spans[k][1]);
                    if (cursor >= span[1])
                    {
                        break;
                    }
                    ++k;
                }

                if (cursor < span[1])
                {
                    row.push_back({ cursor, span[1] });
                }
            }

            if (!row.empty())
            {
                result[y] = std::move(row);
            }
        }

        return result;
    }

    static shape_t translate(const shape_t& shape, location_base_t dx, location_base_t dy)
    {
        shape_t result;
        for (const auto& [y, spans] : shape)
        {
            auto& row = result[y + dy];
            row.reserve(spans.size());
            for (const auto& span : spans)
            {
                row.push_back({ span[0] + dx, span[1] + dx });
            }
        }
        return result;
    }

    static shape_t dilate(const shape_t& shape, location_base_t radius)
    {
        if (radius <= 0)
        {
            return normalize(shape);
        }

        const shape_t base = normalize(shape);
        shape_t result;
        for (location_base_t dy = -radius; dy <= radius; ++dy)
        {
            for (location_base_t dx = -radius; dx <= radius; ++dx)
            {
                result = unite(result, translate(base, dx, dy));
            }
        }
        return result;
    }

    static shape_t erode(const shape_t& shape, location_base_t radius)
    {
        if (radius <= 0)
        {
            return normalize(shape);
        }

        const shape_t base = normalize(shape);
        shape_t result;
        bool init = false;
        for (location_base_t dy = -radius; dy <= radius; ++dy)
        {
            for (location_base_t dx = -radius; dx <= radius; ++dx)
            {
                const shape_t shifted = translate(base, -dx, -dy);
                if (!init)
                {
                    result = shifted;
                    init = true;
                }
                else
                {
                    result = intersection(result, shifted);
                }

                if (result.empty())
                {
                    return result;
                }
            }
        }
        return result;
    }

    static raster_t normalize_disjoint(const raster_t& raster)
    {
        return raster_t{ normalize_disjoint(flatten(raster.m_shapes)) };
    }

    static shape_list_t normalize_disjoint(const shape_t& shape)
    {
        struct row_t
        {
            location_base_t y = 0;
            std::vector<interval_type> spans;
            std::vector<std::size_t> ids;
        };

        const shape_t norm = normalize(shape);
        if (norm.empty())
        {
            return {};
        }

        std::vector<row_t> rows;
        rows.reserve(norm.size());

        std::size_t next_id = 0;
        for (const auto& [y, spans] : norm)
        {
            row_t row;
            row.y = y;
            row.spans = spans;
            row.ids.resize(spans.size());
            for (auto& id : row.ids)
            {
                id = next_id++;
            }
            rows.push_back(std::move(row));
        }

        std::vector<std::size_t> parent(next_id);
        std::iota(parent.begin(), parent.end(), 0);

        const auto find_root = [&](std::size_t id)
        {
            std::size_t root = id;
            while (parent[root] != root)
            {
                root = parent[root];
            }
            while (parent[id] != id)
            {
                const std::size_t next = parent[id];
                parent[id] = root;
                id = next;
            }
            return root;
        };

        const auto unite_ids = [&](std::size_t lhs_id, std::size_t rhs_id)
        {
            const std::size_t lhs_root = find_root(lhs_id);
            const std::size_t rhs_root = find_root(rhs_id);
            if (lhs_root != rhs_root)
            {
                parent[rhs_root] = lhs_root;
            }
        };

        for (std::size_t row_i = 1; row_i < rows.size(); ++row_i)
        {
            auto& prev = rows[row_i - 1];
            auto& curr = rows[row_i];
            if (curr.y - prev.y != 1)
            {
                continue;
            }

            std::size_t j = 0;
            for (std::size_t i = 0; i < prev.spans.size(); ++i)
            {
                const auto& a = prev.spans[i];
                while (j < curr.spans.size() && curr.spans[j][1] < a[0])
                {
                    ++j;
                }

                std::size_t k = j;
                while (k < curr.spans.size() && curr.spans[k][0] <= a[1])
                {
                    unite_ids(prev.ids[i], curr.ids[k]);
                    ++k;
                }
            }
        }

        std::map<std::size_t, shape_t> grouped;
        for (const auto& row : rows)
        {
            for (std::size_t i = 0; i < row.spans.size(); ++i)
            {
                const auto root = find_root(row.ids[i]);
                grouped[root][row.y].push_back(row.spans[i]);
            }
        }

        shape_list_t result;
        result.reserve(grouped.size());
        for (auto& [_, component] : grouped)
        {
            result.push_back(normalize(component));
        }
        return result;
    }
};

}  // namespace mat
}  // namespace zx

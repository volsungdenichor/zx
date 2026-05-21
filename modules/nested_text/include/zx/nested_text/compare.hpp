#pragma once

#include <zx/nested_text/node.hpp>

namespace zx
{

namespace nested_text
{

namespace detail
{
struct compare_fn
{
    struct info_t
    {
        std::string lhs;
        std::string rhs;
    };

    static node_t format(const node_t& node) { return map_t{ { "type", str(node.type()) }, { "value", node } }; }

    static void compare_string(list_t& errors, list_t path, const string_t& lhs, const string_t& rhs, const info_t& info)
    {
        if (lhs != rhs)
        {
            errors.push_back(map_t{ { "path", path }, { "error", "value mismatch" }, { info.lhs, lhs }, { info.rhs, rhs } });
        }
    }

    static void compare_list(list_t& errors, list_t path, const list_t& lhs, const list_t& rhs, const info_t& info)
    {
        if (lhs.size() != rhs.size())
        {
            errors.push_back(map_t{
                { "path", path }, { "error", "list size mismatch" }, { info.lhs, format(lhs) }, { info.rhs, format(rhs) } });
        }
        else
        {
            for (std::size_t i = 0; i < lhs.size(); ++i)
            {
                compare(errors, path + node_t{ str(i) }, lhs[i], rhs[i], info);
            }
        }
    }

    static void compare_map(list_t& errors, list_t path, const map_t& lhs, const map_t& rhs, const info_t& info)
    {
        for (const auto& [key, lhs_value] : lhs)
        {
            if (const auto it = rhs.find(key); it != rhs.end())
            {
                compare(errors, path + node_t{ key }, lhs_value, it->second, info);
            }
            else
            {
                errors.push_back(map_t{ { "path", path }, { "error", "key missing" }, { "key", key } });
            }
        }
        for (const auto& [key, _] : rhs)
        {
            if (!lhs.contains(key))
            {
                errors.push_back(map_t{ { "path", path }, { "error", "extra key" }, { "key", key } });
            }
        }
    }

    static void compare(list_t& errors, list_t path, const node_t& lhs, const node_t& rhs, const info_t& info)
    {
        if (lhs.type() != rhs.type())
        {
            errors.push_back(map_t{
                { "path", path }, { "error", "type mismatch" }, { info.lhs, format(lhs) }, { info.rhs, format(rhs) } });
        }
        else if (lhs.type() == node_type_t::string)
        {
            compare_string(errors, path, lhs.as_string(), rhs.as_string(), info);
        }
        else if (lhs.type() == node_type_t::list)
        {
            compare_list(errors, path, lhs.as_list(), rhs.as_list(), info);
        }
        else if (lhs.type() == node_type_t::map)
        {
            compare_map(errors, path, lhs.as_map(), rhs.as_map(), info);
        }
    }

    list_t operator()(const node_t& lhs, const node_t& rhs, std::string lhs_name = "lhs", std::string rhs_name = "rhs") const
    {
        list_t errors = {};
        compare(errors, {}, lhs, rhs, info_t{ std::move(lhs_name), std::move(rhs_name) });
        return errors;
    }
};

}  // namespace detail

static constexpr inline auto compare = detail::compare_fn{};

}  // namespace nested_text
}  // namespace zx

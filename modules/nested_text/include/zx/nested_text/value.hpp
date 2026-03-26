#pragma once

#include <algorithm>
#include <functional>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <zx/format.hpp>
#include <zx/maybe.hpp>
#include <zx/type_name.hpp>

namespace zx
{

namespace nested_text
{

using path_item_t = std::variant<std::string, std::size_t>;

struct path_t : public std::vector<path_item_t>
{
    path_t() = default;
    path_t(const path_t&) = default;
    path_t(path_t&&) noexcept = default;

    explicit path_t(std::initializer_list<path_item_t> items) : std::vector<path_item_t>(items) { }

    friend std::ostream& operator<<(std::ostream& os, const path_t& item)
    {
        for (std::size_t i = 0; i < item.size(); ++i)
        {
            const auto& it = item[i];

            if (std::holds_alternative<std::string>(it))
            {
                os << (i > 0 ? "." : "") << std::get<std::string>(it);
            }
            else
            {
                os << "[" << std::get<std::size_t>(it) << "]";
            }
        }
        return os;
    }

    static path_t parse(std::string_view text)
    {
        path_t result;

        std::size_t pos = 0;
        while (pos < text.size())
        {
            if (text[pos] == '.')
            {
                ++pos;
            }
            else if (text[pos] == '[')
            {
                ++pos;
                std::size_t end_pos = text.find(']', pos);
                if (end_pos == std::string_view::npos)
                {
                    throw std::runtime_error{ str("Invalid path: missing closing ']' in '", text, "'") };
                }
                const auto index_str = text.substr(pos, end_pos - pos);
                try
                {
                    std::size_t index = std::stoul(std::string(index_str));
                    result.emplace_back(index);
                }
                catch (const std::exception&)
                {
                    throw std::runtime_error{ str("Invalid path: invalid list index '", index_str, "' in '", text, "'") };
                }
                pos = end_pos + 1;
            }
            else
            {
                std::size_t end_pos = text.find_first_of(".[", pos);
                if (end_pos == std::string_view::npos)
                {
                    end_pos = text.size();
                }
                const auto key = text.substr(pos, end_pos - pos);
                result.emplace_back(std::string(key));
                pos = end_pos;
            }
        }

        return result;
    }
};

template <class K, class V>
struct ordered_map
{
    using key_type = K;
    using mapped_type = V;
    using value_type = std::pair<key_type, mapped_type>;

    std::vector<value_type> m_items;

    using iterator = typename std::vector<value_type>::iterator;
    using const_iterator = typename std::vector<value_type>::const_iterator;

    ordered_map() = default;
    ordered_map(const ordered_map&) = default;
    ordered_map(ordered_map&&) noexcept = default;

    ordered_map(std::initializer_list<value_type> items)
    {
        for (const auto& v : items)
        {
            insert(v);
        }
    }

    constexpr iterator begin() { return m_items.begin(); }
    constexpr iterator end() { return m_items.end(); }
    constexpr const_iterator begin() const { return m_items.begin(); }
    constexpr const_iterator end() const { return m_items.end(); }
    constexpr const_iterator cbegin() const { return begin(); }
    constexpr const_iterator cend() const { return end(); }
    constexpr bool empty() const { return m_items.empty(); }
    constexpr std::size_t size() const { return m_items.size(); }

    const_iterator find(const key_type& key) const
    {
        return std::find_if(begin(), end(), [&](const auto& p) { return p.first == key; });
    }

    iterator find(const key_type& key)
    {
        return std::find_if(begin(), end(), [&](const auto& p) { return p.first == key; });
    }

    mapped_type& operator[](const key_type& key)
    {
        auto it = find(key);
        if (it != end())
        {
            return it->second;
        }
        m_items.emplace_back(key, mapped_type{});
        return m_items.back().second;
    }

    std::pair<iterator, bool> emplace(const key_type& key, const mapped_type& value)
    {
        auto it = find(key);
        if (it != end())
        {
            return std::make_pair(it, false);
        }
        m_items.emplace_back(key, value);
        return std::make_pair(std::prev(end()), true);
    }

    std::pair<iterator, bool> insert(const value_type& p) { return emplace(p.first, p.second); }

    const mapped_type& at(const key_type& key) const
    {
        auto it = find(key);
        if (it == end())
        {
            throw std::out_of_range{ str("key '", key, "' not found in map_t") };
        }
        return it->second;
    }

    friend constexpr bool operator==(const ordered_map& lhs, const ordered_map& rhs) { return lhs.m_items == rhs.m_items; }
    friend constexpr bool operator!=(const ordered_map& lhs, const ordered_map& rhs) { return !(lhs == rhs); }
    friend constexpr bool operator<(const ordered_map& lhs, const ordered_map& rhs) { return lhs.m_items < rhs.m_items; }
    friend constexpr bool operator>(const ordered_map& lhs, const ordered_map& rhs) { return rhs < lhs; }
    friend constexpr bool operator<=(const ordered_map& lhs, const ordered_map& rhs) { return !(rhs < lhs); }
    friend constexpr bool operator>=(const ordered_map& lhs, const ordered_map& rhs) { return !(lhs < rhs); }
};

template <class T>
struct box_t
{
    using value_type = T;
    std::unique_ptr<value_type> m_ptr;

    box_t(const value_type& value) : m_ptr(std::make_unique<T>(value)) { }

    box_t(value_type&& value) : m_ptr(std::make_unique<T>(std::move(value))) { }

    box_t(const box_t& other) : box_t(other.get()) { }

    box_t(box_t&&) noexcept = default;

    box_t& operator=(box_t other) noexcept
    {
        std::swap(m_ptr, other.m_ptr);
        return *this;
    }

    constexpr const value_type& get() const& noexcept { return *m_ptr; }

    constexpr operator const value_type&() const noexcept { return get(); }

    friend std::ostream& operator<<(std::ostream& os, const box_t& item) { return os << item.get(); }
};

struct unbox_fn
{
    template <class T>
    constexpr const T& operator()(const box_t<T>& box) const noexcept
    {
        return box.get();
    }

    template <class T>
    constexpr T& operator()(box_t<T>& box) const noexcept
    {
        return box.get();
    }

    template <class T>
    constexpr const T& operator()(const T& value) const noexcept
    {
        return value;
    }

    template <class T>
    constexpr T& operator()(T& value) const noexcept
    {
        return value;
    }
};

static constexpr inline auto unbox = unbox_fn{};

template <class Visitor>
struct unboxing_visitor_t
{
    Visitor m_visitor;

    constexpr unboxing_visitor_t(Visitor visitor) : m_visitor(std::move(visitor)) { }

    template <class... Args>
    constexpr auto operator()(Args&&... args) const -> decltype(std::invoke(m_visitor, unbox(std::forward<Args>(args))...))
    {
        return std::invoke(m_visitor, unbox(std::forward<Args>(args))...);
    }
};

template <class Visitor>
unboxing_visitor_t(Visitor&&) -> unboxing_visitor_t<std::decay_t<Visitor>>;

struct value_t;

using string_t = std::string;

struct list_t : public std::vector<value_t>
{
    using base_t = std::vector<value_t>;
    using base_t::base_t;

    friend std::ostream& operator<<(std::ostream& os, const list_t& item);
};

struct map_t : public ordered_map<string_t, value_t>
{
    using base_t = ordered_map<string_t, value_t>;
    using base_t::base_t;

    friend std::ostream& operator<<(std::ostream& os, const map_t& item);
};

enum class value_type_t
{
    string,
    list,
    map,
};

inline std::ostream& operator<<(std::ostream& os, const value_type_t item)
{
    switch (item)
    {
        case value_type_t::string: os << "string"; break;
        case value_type_t::list: os << "list"; break;
        case value_type_t::map: os << "map"; break;
    }

    return os;
}

struct value_t
{
    using data_type = std::variant<string_t, box_t<list_t>, box_t<map_t>>;

    data_type m_data;

    value_t() : value_t{ "" } { }
    value_t(string_t value) : m_data(std::move(value)) { }
    value_t(const char* value) : m_data(std::string(value)) { }
    value_t(list_t value) : m_data(std::move(value)) { }
    value_t(map_t value) : m_data(std::move(value)) { }

    value_t(const value_t&) = default;
    value_t(value_t&&) noexcept = default;

    value_t& operator=(const value_t& other) = default;
    value_t& operator=(value_t&&) noexcept = default;

    value_type_t type() const
    {
        switch (m_data.index())
        {
            case 0: return value_type_t::string;
            case 1: return value_type_t::list;
            case 2: return value_type_t::map;
            default: throw std::logic_error{ "Invalid variant index" };
        }
    }

    friend std::ostream& operator<<(std::ostream& os, const value_t& item);

    friend constexpr bool operator==(const value_t& lhs, const value_t& rhs);
    friend constexpr bool operator!=(const value_t& lhs, const value_t& rhs);
    friend constexpr bool operator<(const value_t& lhs, const value_t& rhs);
    friend constexpr bool operator>(const value_t& lhs, const value_t& rhs);
    friend constexpr bool operator<=(const value_t& lhs, const value_t& rhs);
    friend constexpr bool operator>=(const value_t& lhs, const value_t& rhs);

    constexpr const string_t* if_string() const { return std::get_if<string_t>(&m_data); }

    constexpr const string_t& as_string() const
    {
        if (const auto maybe_string = if_string())
        {
            return *maybe_string;
        }
        throw std::runtime_error{ zx::str("expected type: string, actual: ", type()) };
    }

    constexpr const list_t* if_list() const
    {
        if (auto ptr = std::get_if<box_t<list_t>>(&m_data))
        {
            return &ptr->get();
        }
        return nullptr;
    }

    constexpr const list_t& as_list() const
    {
        if (const auto maybe_list = if_list())
        {
            return *maybe_list;
        }
        throw std::runtime_error{ zx::str("expected type: list, actual: ", type()) };
    }

    constexpr const map_t* if_map() const
    {
        if (auto ptr = std::get_if<box_t<map_t>>(&m_data))
        {
            return &ptr->get();
        }
        return nullptr;
    }

    constexpr const map_t& as_map() const
    {
        if (const auto maybe_map = if_map())
        {
            return *maybe_map;
        }
        throw std::runtime_error{ zx::str("expected type: map, actual: ", type()) };
    }

    maybe_t<const value_t&> get(const path_t& path) const
    {
        const value_t* current = this;
        for (const path_item_t& item : path)
        {
            if (std::holds_alternative<std::string>(item))
            {
                const std::string& key = std::get<std::string>(item);
                if (const map_t* map = current->if_map())
                {
                    if (auto it = map->find(key); it != map->end())
                    {
                        current = &it->second;
                        continue;
                    }

                    return none;
                }
                else
                {
                    return none;
                }
            }
            else
            {
                std::size_t index = std::get<std::size_t>(item);
                if (const list_t* list = current->if_list())
                {
                    if (index < list->size())
                    {
                        current = &(*list)[index];
                        continue;
                    }

                    return none;
                }
                else
                {
                    return none;
                }
            }
        }
        return current ? maybe_t<const value_t&>{ *current } : none;
    }
};

inline std::ostream& operator<<(std::ostream& os, const list_t& item)
{
    os << "[";
    for (auto it = item.begin(); it != item.end(); ++it)
    {
        if (it != item.begin())
        {
            os << " ";
        }
        os << *it;
    }
    os << "]";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const map_t& item)
{
    os << "{";
    for (auto it = item.begin(); it != item.end(); ++it)
    {
        if (it != item.begin())
        {
            os << " ";
        }
        os << ":" << it->first << " " << it->second;
    }
    os << "}";
    return os;
}

namespace detail
{

template <class T>
value_type_t type(const T&)
{
    if constexpr (std::is_same_v<T, string_t>)
    {
        return value_type_t::string;
    }
    else if constexpr (std::is_same_v<T, list_t>)
    {
        return value_type_t::list;
    }
    else if constexpr (std::is_same_v<T, map_t>)
    {
        return value_type_t::map;
    }
}

struct print_visitor_t
{
    std::ostream& os;

    void operator()(const string_t& v) const
    {
        if (v.empty() || std::any_of(v.begin(), v.end(), [](char ch) { return std::isspace(ch) || ch == '"'; }))
        {
            os << std::quoted(v);
        }
        else
        {
            os << v;
        }
    }
    void operator()(const list_t& v) const { os << v; }
    void operator()(const map_t& v) const { os << v; }
};

struct eq_visitor_t
{
    bool operator()(const string_t& lhs, const string_t& rhs) const { return lhs == rhs; }
    bool operator()(const list_t& lhs, const list_t& rhs) const { return lhs == rhs; }
    bool operator()(const map_t& lhs, const map_t& rhs) const { return lhs == rhs; }

    template <class L, class R>
    bool operator()(const L&, const R&) const
    {
        return false;
    }
};

struct lt_visitor_t
{
    bool operator()(const string_t& lhs, const string_t& rhs) const { return lhs < rhs; }
    bool operator()(const list_t& lhs, const list_t& rhs) const { return lhs < rhs; }
    bool operator()(const map_t& lhs, const map_t& rhs) const { return lhs < rhs; }

    template <class L, class R>
    bool operator()(const L& lhs, const R& rhs) const
    {
        return type(lhs) < type(rhs);
    }
};

}  // namespace detail

inline std::ostream& operator<<(std::ostream& os, const value_t& item)
{
    std::visit(unboxing_visitor_t{ detail::print_visitor_t{ os } }, item.m_data);
    return os;
}

inline constexpr bool operator==(const value_t& lhs, const value_t& rhs)
{
    return std::visit(unboxing_visitor_t{ detail::eq_visitor_t{} }, lhs.m_data, rhs.m_data);
}

inline constexpr bool operator<(const value_t& lhs, const value_t& rhs)
{
    return std::visit(unboxing_visitor_t{ detail::lt_visitor_t{} }, lhs.m_data, rhs.m_data);
}

inline constexpr bool operator>(const value_t& lhs, const value_t& rhs)
{
    return rhs < lhs;
}

inline constexpr bool operator<=(const value_t& lhs, const value_t& rhs)
{
    return !(lhs > rhs);
}

inline constexpr bool operator>=(const value_t& lhs, const value_t& rhs)
{
    return !(lhs < rhs);
}

template <class... Args>
value_t text(const Args&... args)
{
    return str(args...);
}

}  // namespace nested_text

}  // namespace zx

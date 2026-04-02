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

inline bool is_space(char ch)
{
    return std::isspace(static_cast<unsigned char>(ch));
}

template <class K, class V>
struct ordered_map
{
    using key_type = K;
    using mapped_type = V;
    using value_type = std::pair<key_type, mapped_type>;
    using size_type = std::size_t;

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
    constexpr size_type size() const { return m_items.size(); }

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

    bool contains(const key_type& key) const { return find(key) != end(); }
    size_type count(const key_type& key) const { return contains(key) ? 1 : 0; }

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

struct node_t;

using string_t = std::string;

struct list_t : public std::vector<node_t>
{
    using base_t = std::vector<node_t>;
    using base_t::base_t;

    friend std::ostream& operator<<(std::ostream& os, const list_t& item);
};

struct map_t : public ordered_map<string_t, node_t>
{
    using base_t = ordered_map<string_t, node_t>;
    using base_t::base_t;

    friend std::ostream& operator<<(std::ostream& os, const map_t& item);
};

enum class node_type_t
{
    string,
    list,
    map,
};

inline std::ostream& operator<<(std::ostream& os, const node_type_t item)
{
    switch (item)
    {
        case node_type_t::string: os << "string"; break;
        case node_type_t::list: os << "list"; break;
        case node_type_t::map: os << "map"; break;
    }

    return os;
}

struct node_t
{
    using data_type = std::variant<string_t, box_t<list_t>, box_t<map_t>>;

    data_type m_data;

    node_t() : node_t{ "" } { }
    node_t(string_t value) : m_data(std::move(value)) { }
    node_t(const char* value) : m_data(std::string(value)) { }
    node_t(list_t value) : m_data(std::move(value)) { }
    node_t(map_t value) : m_data(std::move(value)) { }

    node_t(const node_t&) = default;
    node_t(node_t&&) noexcept = default;

    node_t& operator=(const node_t& other) = default;
    node_t& operator=(node_t&&) noexcept = default;

    node_type_t type() const
    {
        switch (m_data.index())
        {
            case 0: return node_type_t::string;
            case 1: return node_type_t::list;
            case 2: return node_type_t::map;
            default: throw std::logic_error{ "Invalid variant index" };
        }
    }

    friend std::ostream& operator<<(std::ostream& os, const node_t& item);

    friend constexpr bool operator==(const node_t& lhs, const node_t& rhs);
    friend constexpr bool operator!=(const node_t& lhs, const node_t& rhs);
    friend constexpr bool operator<(const node_t& lhs, const node_t& rhs);
    friend constexpr bool operator>(const node_t& lhs, const node_t& rhs);
    friend constexpr bool operator<=(const node_t& lhs, const node_t& rhs);
    friend constexpr bool operator>=(const node_t& lhs, const node_t& rhs);

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

    maybe_t<const node_t&> get(const list_t& path) const
    {
        const node_t* current = this;
        for (const node_t& item : path)
        {
            const string_t& key = item.as_string();
            if (const map_t* map = current->if_map())
            {
                if (const auto it = map->find(key); it != map->end())
                {
                    current = &it->second;
                    continue;
                }
                else
                {
                    return none;
                }
            }
            else if (const list_t* list = current->if_list())
            {
                const maybe_t<std::size_t> index = std::invoke(
                    [&]() -> maybe_t<std::size_t>
                    {
                        try
                        {
                            return std::stoul(key);
                        }
                        catch (const std::exception&)
                        {
                            return none;
                        }
                    });
                if (index && *index < list->size())
                {
                    current = &(*list)[*index];
                    continue;
                }
                else
                {
                    return none;
                }
            }
            else
            {
                return none;
            }
        }
        return current ? maybe_t<const node_t&>{ *current } : none;
    }
};

namespace detail
{

template <class T>
node_type_t type(const T&)
{
    if constexpr (std::is_same_v<T, string_t>)
    {
        return node_type_t::string;
    }
    else if constexpr (std::is_same_v<T, list_t>)
    {
        return node_type_t::list;
    }
    else if constexpr (std::is_same_v<T, map_t>)
    {
        return node_type_t::map;
    }
}

struct print_visitor_t
{
    std::ostream& os;

    void operator()(const string_t& item) const
    {
        if (item.empty()
            || std::any_of(
                item.begin(),
                item.end(),
                [](char ch) { return is_space(ch) || std::string_view("\"[]{}").find(ch) != std::string_view::npos; }))
        {
            os << std::quoted(item);
        }
        else
        {
            os << item;
        }
    }

    void operator()(const list_t& item) const
    {
        os << "[";
        for (auto it = item.begin(); it != item.end(); ++it)
        {
            if (it != item.begin())
            {
                os << " ";
            }
            (*this)(*it);
        }
        os << "]";
    }

    void operator()(const map_t& item) const
    {
        os << "{";
        for (auto it = item.begin(); it != item.end(); ++it)
        {
            if (it != item.begin())
            {
                os << " ";
            }
            os << ":" << it->first << " ";
            (*this)(it->second);
        }
        os << "}";
    }

    void operator()(const node_t& item) const { std::visit(unboxing_visitor_t{ *this }, item.m_data); }
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

inline std::ostream& operator<<(std::ostream& os, const list_t& item)
{
    detail::print_visitor_t{ os }(item);
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const map_t& item)
{
    detail::print_visitor_t{ os }(item);
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const node_t& item)
{
    detail::print_visitor_t{ os }(item);
    return os;
}

inline constexpr bool operator==(const node_t& lhs, const node_t& rhs)
{
    return std::visit(unboxing_visitor_t{ detail::eq_visitor_t{} }, lhs.m_data, rhs.m_data);
}

inline constexpr bool operator<(const node_t& lhs, const node_t& rhs)
{
    return std::visit(unboxing_visitor_t{ detail::lt_visitor_t{} }, lhs.m_data, rhs.m_data);
}

inline constexpr bool operator>(const node_t& lhs, const node_t& rhs)
{
    return rhs < lhs;
}

inline constexpr bool operator<=(const node_t& lhs, const node_t& rhs)
{
    return !(lhs > rhs);
}

inline constexpr bool operator>=(const node_t& lhs, const node_t& rhs)
{
    return !(lhs < rhs);
}

template <class... Args>
node_t text(const Args&... args)
{
    return str(args...);
}

}  // namespace nested_text

}  // namespace zx

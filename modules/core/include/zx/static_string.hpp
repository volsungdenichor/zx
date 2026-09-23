#pragma once

#include <array>
#include <iostream>
#include <string_view>

namespace zx
{

template <char... Ch>
struct static_string_t
{
private:
    static constexpr std::array<char, sizeof...(Ch) + 1> value{ Ch..., '\0' };

public:
    static constexpr std::string_view get() { return std::string_view{ value.data(), sizeof...(Ch) }; }

    friend std::ostream& operator<<(std::ostream& os, const static_string_t& item) { return os << item.get(); }

    friend bool operator==(const static_string_t&, const static_string_t&) { return true; }
    friend bool operator!=(const static_string_t&, const static_string_t&) { return false; }
    friend bool operator<(const static_string_t&, const static_string_t&) { return false; }
    friend bool operator<=(const static_string_t&, const static_string_t&) { return true; }
    friend bool operator>(const static_string_t&, const static_string_t&) { return false; }
    friend bool operator>=(const static_string_t&, const static_string_t&) { return true; }

    friend bool operator==(const static_string_t& lhs, const std::string_view& rhs) { return lhs.get() == rhs; }
    friend bool operator!=(const static_string_t& lhs, const std::string_view& rhs) { return lhs.get() != rhs; }
    friend bool operator<(const static_string_t& lhs, const std::string_view& rhs) { return lhs.get() < rhs; }
    friend bool operator<=(const static_string_t& lhs, const std::string_view& rhs) { return lhs.get() <= rhs; }
    friend bool operator>(const static_string_t& lhs, const std::string_view& rhs) { return lhs.get() > rhs; }
    friend bool operator>=(const static_string_t& lhs, const std::string_view& rhs) { return lhs.get() >= rhs; }

    friend bool operator==(const std::string_view& lhs, const static_string_t& rhs) { return lhs == rhs.get(); }
    friend bool operator!=(const std::string_view& lhs, const static_string_t& rhs) { return lhs != rhs.get(); }
    friend bool operator<(const std::string_view& lhs, const static_string_t& rhs) { return lhs < rhs.get(); }
    friend bool operator<=(const std::string_view& lhs, const static_string_t& rhs) { return lhs <= rhs.get(); }
    friend bool operator>(const std::string_view& lhs, const static_string_t& rhs) { return lhs > rhs.get(); }
    friend bool operator>=(const std::string_view& lhs, const static_string_t& rhs) { return lhs >= rhs.get(); }
};

template <class L, class R>
struct concat_static_string_impl_t;

template <class L, class R>
using concat_static_string_t = typename concat_static_string_impl_t<L, R>::type;

template <char... L, char... R>
struct concat_static_string_impl_t<static_string_t<L...>, static_string_t<R...>>
{
    using type = static_string_t<L..., R...>;
};

template <char... L, char... R>
constexpr auto operator+(const static_string_t<L...>&, const static_string_t<R...>&)
    -> concat_static_string_t<static_string_t<L...>, static_string_t<R...>>
{
    return {};
}

}  // namespace zx
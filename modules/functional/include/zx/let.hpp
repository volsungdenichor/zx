#pragma once

#include <functional>
#include <tuple>
#include <utility>

namespace zx
{

namespace detail
{

template <class Tuple>
struct let_result;

template <class... Args>
struct let_result<std::tuple<Args...>>
{
private:
    static constexpr std::size_t args_count = sizeof...(Args);
    using tuple_type = std::tuple<Args...>;
    using func_type = std::tuple_element_t<args_count - 1, tuple_type>;
    using index_sequence = std::make_index_sequence<args_count - 1>;

    template <std::size_t... I>
    static auto make_type(std::index_sequence<I...>)
        -> std::invoke_result_t<func_type, std::tuple_element_t<I, tuple_type>...>;

public:
    using type = decltype(make_type(index_sequence{}));
};

template <class... Args>
using let_result_t = typename let_result<std::tuple<Args...>>::type;

struct let_fn
{
    template <class... Args>
    constexpr auto operator()(Args&&... args) const -> let_result_t<Args&&...>
    {
        constexpr std::size_t args_count = sizeof...(Args);
        return impl<args_count - 1>(
            std::make_index_sequence<args_count - 1>{}, std::forward_as_tuple(std::forward<Args>(args)...));
    }

private:
    template <std::size_t Last, std::size_t... I, class Tuple>
    static constexpr decltype(auto) impl(std::index_sequence<I...>, Tuple&& tuple)
    {
        return std::invoke(std::get<Last>(std::forward<Tuple>(tuple)), std::get<I>(std::forward<Tuple>(tuple))...);
    }
};

}  // namespace detail

static constexpr detail::let_fn let;

}  // namespace zx

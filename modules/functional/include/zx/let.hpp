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
    template <std::size_t... I>
    static auto make_type(std::index_sequence<I...>)
        -> std::invoke_result_t<
            std::tuple_element_t<sizeof...(Args) - 1, std::tuple<Args...>>,
            std::tuple_element_t<I, std::tuple<Args...>>...>;

public:
    using type = decltype(make_type(std::make_index_sequence<sizeof...(Args) - 1>{}));
};

template <class... Args>
using let_result_t = typename let_result<std::tuple<Args...>>::type;

struct let_fn
{
    template <class... Args>
    constexpr auto operator()(Args&&... args) const -> let_result_t<Args&&...>
    {
        static_assert(sizeof...(Args) >= 1, "zx::let requires a callable");
        return impl(std::make_index_sequence<sizeof...(Args) - 1>{}, std::forward<Args>(args)...);
    }

private:
    template <std::size_t... I, class... Args>
    static constexpr decltype(auto) impl(std::index_sequence<I...>, Args&&... args)
    {
        auto tuple = std::forward_as_tuple(std::forward<Args>(args)...);
        return std::invoke(
            std::get<sizeof...(Args) - 1>(std::move(tuple)),
            std::get<I>(std::move(tuple))...);
    }
};

}  // namespace detail

static constexpr detail::let_fn let;

}  // namespace zx

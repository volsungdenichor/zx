#pragma once

#include <functional>

namespace zx
{

template <class Signature>
struct function_ref;

template <class Ret, class... Args>
struct function_ref<Ret(Args...)>
{
    using return_type = Ret;
    using func_type = return_type (*)(void*, Args...);

    void* m_obj;
    func_type m_func;

    constexpr function_ref() = delete;
    constexpr function_ref(const function_ref&) = default;

    template <class Func>
    constexpr function_ref(Func&& func)
        : m_obj{ const_cast<void*>(reinterpret_cast<const void*>(std::addressof(func))) }
        , m_func{ [](void* obj, Args... args) -> return_type
                  { return std::invoke(*static_cast<std::add_pointer_t<Func>>(obj), std::forward<Args>(args)...); } }
    {
    }

    constexpr function_ref& operator=(const function_ref&) = default;

    template <class Func>
    constexpr function_ref& operator=(Func&& func)
    {
        m_obj = const_cast<void*>(reinterpret_cast<const void*>(std::addressof(func)));
        m_func = [](void* obj, Args... args) -> return_type
        { return std::invoke(*static_cast<std::add_pointer_t<Func>>(obj), std::forward<Args>(args)...); };
        return *this;
    }

    constexpr auto operator()(Args... args) const -> return_type
    {
        return m_func(m_obj, std::forward<Args>(args)...);
    }
};

}  // namespace zx
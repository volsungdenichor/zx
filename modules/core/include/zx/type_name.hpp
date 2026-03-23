#pragma once

#include <string>
#include <typeinfo>

#if defined(_WIN32)

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <dbghelp.h>

#elif defined(__has_include)

#if __has_include(<cxxabi.h>)
#include <cxxabi.h>

#include <cstdlib>
#include <memory>
#endif

#endif

namespace zx
{

namespace detail
{

inline std::string demangle(const char* name)
{
    if (name == nullptr)
    {
        return {};
    }

#if defined(_WIN32)
    std::string result(1024, '\0');
    const auto length = UnDecorateSymbolName(name, result.data(), static_cast<DWORD>(result.size()), UNDNAME_COMPLETE);
    if (length == 0)
    {
        return name;
    }

    result.resize(length);
    return result;
#elif defined(__has_include)
#if __has_include(<cxxabi.h>)
    int status = -4;
    std::unique_ptr<char, void (*)(void*)> res{ abi::__cxa_demangle(name, nullptr, nullptr, &status), std::free };
    return status == 0 ? res.get() : name;
#else
    return name;
#endif
#else
    return name;
#endif
}

}  // namespace detail

template <class T>
std::string_view type_name()
{
    static const std::string result = detail::demangle(typeid(T).name());
    return result;
}

template <class T>
std::string_view type_name(const T&)
{
    return type_name<T>();
}
}  // namespace zx

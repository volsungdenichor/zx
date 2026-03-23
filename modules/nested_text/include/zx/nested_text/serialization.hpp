#pragma once

#include <zx/nested_text/value.hpp>

namespace zx
{
namespace nested_text
{

template <class T, class = void>
struct codec_t
{
};

template <class T>
value_t encode(const T& value)
{
    static const codec_t<T> codec = {};
    return codec.encode(value);
}

template <class T>
T decode(const value_t& value)
{
    static const codec_t<T> codec = {};
    return codec.decode(value);
}

template <>
struct codec_t<std::string>
{
    value_t encode(const std::string& value) const { return value; }

    std::string decode(const value_t& value) const
    {
        if (const auto maybe_string = value.if_string())
        {
            return *maybe_string;
        }
        throw std::runtime_error{ "Expected string" };
    }
};

template <class T>
struct ostream_codec_t
{
    value_t encode(const T& value) const
    {
        std::ostringstream os;
        os << value;
        return os.str();
    }

    T decode(const value_t& value) const
    {
        if (const auto maybe_string = value.if_string())
        {
            std::istringstream is(*maybe_string);
            T result;
            is >> result;
            if (is)
            {
                return result;
            }
        }
        throw std::runtime_error{ "Expected string that can be decoded to the target type" };
    }
};

template <class T>
struct codec_t<T, std::enable_if_t<std::is_integral_v<T>>> : ostream_codec_t<T>
{
    static_assert(std::is_integral_v<T>, "codec_t<T> requires T to be an integral type");
};

}  // namespace nested_text
}  // namespace zx

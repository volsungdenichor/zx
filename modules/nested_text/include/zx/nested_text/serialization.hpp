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
    value_t encode(const std::string& in) const { return in; }

    std::string decode(const value_t& in) const
    {
        if (const auto maybe_string = in.if_string())
        {
            return *maybe_string;
        }
        throw std::runtime_error{ "Expected string" };
    }
};

template <class T>
struct ostream_codec_t
{
    value_t encode(const T& in) const
    {
        std::ostringstream os;
        os << in;
        return os.str();
    }

    T decode(const value_t& in) const
    {
        if (const auto maybe_string = in.if_string())
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

template <class T>
struct codec_t<std::vector<T>>
{
    value_t encode(const std::vector<T>& in) const
    {
        list_t out;
        out.reserve(in.size());
        for (const T& item : in)
        {
            out.push_back(nested_text::encode(item));
        }
        return out;
    }

    std::vector<T> decode(const value_t& in) const
    {
        if (const auto maybe_list = in.if_list())
        {
            std::vector<T> out;
            out.reserve(maybe_list->size());
            for (const value_t& item : *maybe_list)
            {
                out.push_back(nested_text::decode<T>(item));
            }
            return out;
        }
        throw std::runtime_error{ "Expected list" };
    }
};

}  // namespace nested_text
}  // namespace zx

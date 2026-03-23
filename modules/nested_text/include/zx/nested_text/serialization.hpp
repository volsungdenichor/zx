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
    try
    {
        return codec.decode(value);
    }
    catch (const std::exception& e)
    {
        std::throw_with_nested(std::runtime_error{ str("Failed to decode value: ", e.what()) });
    }
}

template <>
struct codec_t<std::string>
{
    value_t encode(const std::string& in) const { return in; }

    std::string decode(const value_t& in) const { return in.as_string(); }
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
        std::istringstream is(in.as_string());
        T result;
        is >> result;
        if (is)
        {
            return result;
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
        const list_t& list = in.as_list();
        std::vector<T> out;
        out.reserve(list.size());
        for (const value_t& item : list)
        {
            out.push_back(nested_text::decode<T>(item));
        }
        return out;
    }
};

}  // namespace nested_text
}  // namespace zx

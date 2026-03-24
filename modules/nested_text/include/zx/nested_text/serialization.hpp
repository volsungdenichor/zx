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

template <class Type, class T>
std::ptrdiff_t offset_of(T Type::*member)
{
    return reinterpret_cast<std::ptrdiff_t>(&(reinterpret_cast<Type*>(0)->*member));
}

template <class T>
struct struct_codec_t
{
    struct member_t
    {
        using encode_fn_t = std::function<value_t(const void*, std::ptrdiff_t)>;
        using decode_fn_t = std::function<void(void*, std::ptrdiff_t, const value_t&)>;

        std::string m_name;
        std::ptrdiff_t m_offset;

        encode_fn_t m_encode_fn;
        decode_fn_t m_decode_fn;

        template <class Type>
        member_t(std::string name, Type T::*member)
            : m_name{ std::move(name) }
            , m_offset{ offset_of(member) }
            , m_encode_fn{ [](const void* obj, std::ptrdiff_t offset) -> value_t {
                return nested_text::encode(*reinterpret_cast<const Type*>(reinterpret_cast<const char*>(obj) + offset));
            } }
            , m_decode_fn{ [](void* obj, std::ptrdiff_t offset, const value_t& value) {
                *reinterpret_cast<Type*>(reinterpret_cast<char*>(obj) + offset) = nested_text::decode<Type>(value);
            } }
        {
        }
    };

    std::vector<member_t> m_members;

    struct_codec_t(std::vector<member_t> members) : m_members(std::move(members)) { }

    value_t encode(const T& in) const
    {
        map_t out;
        for (const member_t& member : m_members)
        {
            out.emplace(member.m_name, member.m_encode_fn(&in, member.m_offset));
        }
        return out;
    }

    T decode(const value_t& in) const
    {
        const map_t& map = in.as_map();
        T out{};
        for (const member_t& member : m_members)
        {
            member.m_decode_fn(&out, member.m_offset, map.at(member.m_name));
        }
        return out;
    }
};

}  // namespace nested_text
}  // namespace zx

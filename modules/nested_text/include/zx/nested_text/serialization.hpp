#pragma once

#include <cstdint>
#include <cstring>
#include <zx/nested_text/node.hpp>

namespace zx
{
namespace nested_text
{

template <class T, class = void>
struct codec_t
{
};

template <class T>
node_t encode(const T& value)
{
    static const codec_t<T> codec = {};
    return codec.encode(value);
}

template <class T>
void encode(node_t& out, const T& in)
{
    out = encode(in);
}

template <class T>
T decode(const node_t& value)
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

template <class T>
void decode(T& out, const node_t& in)
{
    out = decode<T>(in);
}

template <>
struct codec_t<std::string>
{
    node_t encode(const std::string& in) const { return in; }

    std::string decode(const node_t& in) const { return in.as_string(); }
};

template <class T>
struct stream_encoder_t
{
    node_t encode(const T& in) const
    {
        std::ostringstream os;
        os << in;
        return os.str();
    }
};

template <class T>
struct stream_decoder_t
{
    T decode(const node_t& in) const
    {
        const auto s = in.as_string();
        std::istringstream is(s);
        T result{};
        is >> result >> std::ws;
        if (!is.eof())
        {
            throw std::runtime_error{ str("Cannot decode ", type_name<T>(), " from '", s, "'") };
        }
        return result;
    }
};

template <class T>
struct stream_codec_t : stream_encoder_t<T>, stream_decoder_t<T>
{
};

template <class T>
struct codec_t<T, std::enable_if_t<std::is_integral_v<T>>> : stream_codec_t<T>
{
    static_assert(std::is_integral_v<T>, "codec_t<T> requires T to be an integral type");
};

template <class T>
struct codec_t<std::vector<T>>
{
    node_t encode(const std::vector<T>& in) const
    {
        list_t out;
        out.reserve(in.size());
        for (const T& item : in)
        {
            out.push_back(nested_text::encode(item));
        }
        return out;
    }

    std::vector<T> decode(const node_t& in) const
    {
        const list_t& list = in.as_list();
        std::vector<T> out;
        out.reserve(list.size());
        for (const node_t& item : list)
        {
            out.push_back(nested_text::decode<T>(item));
        }
        return out;
    }
};

template <class T>
struct struct_codec_t
{
    struct member_t
    {
        using encode_fn_t = node_t (*)(const T&, const void*);
        using decode_fn_t = void (*)(T&, const node_t&, const void*);

        std::string m_name;
        std::uintptr_t m_member[2];

        encode_fn_t m_encode_fn;
        decode_fn_t m_decode_fn;

        template <class Type>
        member_t(std::string name, Type T::*member)
            : m_name{ std::move(name) }
            , m_member{}
            , m_encode_fn{ &encode_thunk<Type> }
            , m_decode_fn{ &decode_thunk<Type> }
        {
            static_assert(
                sizeof(member) <= sizeof(m_member),
                "Pointer-to-member too large for inline storage; increase m_member size");
            std::memcpy(m_member, &member, sizeof(member));
        }

        template <class Type>
        static node_t encode_thunk(const T& obj, const void* ptr)
        {
            Type T::*member;
            std::memcpy(&member, ptr, sizeof(member));
            return nested_text::encode(obj.*member);
        }

        template <class Type>
        static void decode_thunk(T& obj, const node_t& value, const void* ptr)
        {
            Type T::*member;
            std::memcpy(&member, ptr, sizeof(member));
            obj.*member = nested_text::decode<Type>(value);
        }
    };

    std::vector<member_t> m_members;

    struct_codec_t(std::vector<member_t> members) : m_members(std::move(members)) { }

    node_t encode(const T& in) const
    {
        map_t out;
        for (const member_t& member : m_members)
        {
            out.emplace(member.m_name, member.m_encode_fn(in, &member.m_member));
        }
        return out;
    }

    T decode(const node_t& in) const
    {
        const map_t& map = in.as_map();
        T out{};
        for (const member_t& member : m_members)
        {
            member.m_decode_fn(out, map.at(member.m_name), &member.m_member);
        }
        return out;
    }
};

}  // namespace nested_text
}  // namespace zx

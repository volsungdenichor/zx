#pragma once

#include <map>
#include <unordered_map>
#include <vector>
#include <zx/mat.hpp>
#include <zx/sequence.hpp>

namespace zx
{

namespace geometry
{

template <class... Args>
using generator_t = std::function<void(std::function<bool(Args...)>)>;

namespace detail
{

struct dcel_base_t
{
    using vertex_id_type = int;
    using face_id_type = int;
    using halfedge_id_type = int;

    struct vertex_info_t
    {
        vertex_id_type id;
        halfedge_id_type halfedge = halfedge_id_type{ -1 };

        friend std::ostream& operator<<(std::ostream& os, const vertex_info_t& item)
        {
            return os << "V[" << item.id << "] he=" << item.halfedge;
        }
    };

    struct face_info_t
    {
        face_id_type id;
        halfedge_id_type halfedge = halfedge_id_type{ -1 };

        friend std::ostream& operator<<(std::ostream& os, const face_info_t& item)
        {
            return os << "F[" << item.id << "] he=" << item.halfedge;
        }
    };

    struct halfedge_info_t
    {
        halfedge_id_type id;
        vertex_id_type vertex_from = vertex_id_type{ -1 };
        halfedge_id_type twin_halfedge = halfedge_id_type{ -1 };
        halfedge_id_type next_halfedge = halfedge_id_type{ -1 };
        halfedge_id_type prev_halfedge = halfedge_id_type{ -1 };
        face_id_type face = face_id_type{ -1 };

        friend std::ostream& operator<<(std::ostream& os, const halfedge_info_t& item)
        {
            return os << "HE[" << item.id << "] from_v=" << item.vertex_from << " twin_he=" << item.twin_halfedge << " "
                      << item.prev_halfedge << ">" << item.id << ">" << item.next_halfedge << " F=" << item.face;
        }
    };
};

template <class T>
class dcel_t : public detail::dcel_base_t
{
public:
    using location_type = mat::vector_t<T, 2>;
    using segment_type = mat::segment_t<T, 2>;
    using polygon_type = mat::polygon_t<T, 2>;

    struct vertex_t;
    struct face_t;
    struct halfedge_t;

    dcel_t() : m_vertices{}, m_locations{}, m_faces{}, m_halfedges{}, m_edges{}, m_boundary_halfedge{ -1 } { }

    vertex_id_type add_vertex(const location_type& location)
    {
        vertex_info_t& v = new_vertex();
        set_location(v.id, location);
        return v.id;
    }

    face_id_type add_face(const std::vector<vertex_id_type>& vertices)
    {
        if (vertices.size() < 3)
        {
            throw std::runtime_error{ "add_face: at least 3 vertices required" };
        }
        face_info_t& face = new_face();
        build_face(vertices, &face);
        return face.id;
    }

    void add_boundary() { m_boundary_halfedge = build_face(hull(), nullptr); }

    vertex_t vertex(vertex_id_type id) const { return vertex_t{ this, id }; }
    face_t face(face_id_type id) const { return face_t{ this, id }; }
    halfedge_t halfedge(halfedge_id_type id) const { return halfedge_t{ this, id }; }

    sequence_t<vertex_t> vertices() const
    {
        return zx::seq::view(m_vertices).transform([this](const vertex_info_t& v) { return vertex_t{ this, v.id }; });
    }

    sequence_t<face_t> faces() const
    {
        return zx::seq::view(m_faces).transform([this](const face_info_t& f) { return face_t{ this, f.id }; });
    }

    sequence_t<halfedge_t> halfedges() const
    {
        return zx::seq::view(m_halfedges).transform([this](const halfedge_info_t& h) { return halfedge_t{ this, h.id }; });
    }

    sequence_t<halfedge_t> outer_halfedges() const
    {
        if (m_boundary_halfedge == halfedge_id_type{ -1 })
        {
            return seq::empty<halfedge_t>();
        }

        struct next_fn
        {
            mutable halfedge_t next;
            const halfedge_id_type first_id;
            mutable bool done = false;

            maybe_t<halfedge_t> operator()()
            {
                if (done)
                {
                    return {};
                }

                auto current = next;
                next = next.next_halfedge();
                done = (next.id == first_id);
                return current;
            }
        };

        halfedge_t next = halfedge_t{ this, m_boundary_halfedge };
        return sequence_t<halfedge_t>{ next_fn{ next, next.id } };
    }

    struct vertex_t
    {
        const dcel_t* m_self;
        vertex_id_type id;

        const location_type& location() const { return m_self->get_location(id); }

        sequence_t<halfedge_t> out_halfedges() const
        {
            struct next_fn
            {
                mutable halfedge_t next;
                const halfedge_id_type first_id;
                mutable bool done = false;

                maybe_t<halfedge_t> operator()()
                {
                    if (done)
                    {
                        return {};
                    }

                    auto current = next;
                    next = next.twin_halfedge().next_halfedge();
                    done = (next.id == first_id);
                    return current;
                }
            };

            halfedge_t next = m_self->halfedge(info().halfedge);
            return sequence_t<halfedge_t>{ next_fn{ next, next.id } };
        }

        sequence_t<halfedge_t> in_halfedges() const
        {
            struct next_fn
            {
                mutable halfedge_t next;
                const halfedge_id_type first_id;
                mutable bool done = false;

                maybe_t<halfedge_t> operator()()
                {
                    if (done)
                    {
                        return {};
                    }

                    auto current = next;
                    next = next.next_halfedge().twin_halfedge();
                    done = (next.id == first_id);
                    return current;
                }
            };

            halfedge_t next = m_self->halfedge(info().halfedge).twin_halfedge();
            return sequence_t<halfedge_t>{ next_fn{ next, next.id } };
        }

        sequence_t<face_t> incident_faces() const
        {
            struct next_fn
            {
                mutable halfedge_t next;
                const halfedge_id_type first_id;
                mutable bool done = false;

                maybe_t<face_t> operator()()
                {
                    while (true)
                    {
                        if (done)
                        {
                            return {};
                        }

                        auto current = next;
                        next = next.next_halfedge().twin_halfedge();
                        done = (next.id == first_id);
                        if (auto face = current.incident_face())
                        {
                            return face;
                        }
                    }
                }
            };

            halfedge_t next = m_self->halfedge(info().halfedge).twin_halfedge();
            return sequence_t<face_t>{ next_fn{ next, next.id } };
        }

        friend std::ostream& operator<<(std::ostream& os, const vertex_t& item)
        {
            return os << "V: " << item.id << " " << item.location();
        }

    private:
        const vertex_info_t& info() const { return m_self->get_vertex(id); }
    };

    struct face_t
    {
        const dcel_t* m_self;
        face_id_type id;

        halfedge_t halfedge() const { return halfedge_t{ m_self, info().halfedge }; }

        sequence_t<halfedge_t> halfedges() const
        {
            struct next_fn
            {
                mutable halfedge_t next;
                const halfedge_id_type first_id;
                mutable bool done = false;

                maybe_t<halfedge_t> operator()()
                {
                    if (done)
                    {
                        return {};
                    }

                    auto current = next;
                    next = next.next_halfedge();
                    done = (next.id == first_id);
                    return current;
                }
            };

            halfedge_t next = halfedge();
            return sequence_t<halfedge_t>{ next_fn{ next, next.id } };
        }

        sequence_t<vertex_t> vertices() const
        {
            struct next_fn
            {
                mutable halfedge_t next;
                const halfedge_id_type first_id;
                mutable bool done = false;

                maybe_t<vertex_t> operator()()
                {
                    if (done)
                    {
                        return {};
                    }

                    auto current = next;
                    next = next.next_halfedge();
                    done = (next.id == first_id);
                    return current.vertex_from();
                }
            };

            halfedge_t next = halfedge();
            return sequence_t<vertex_t>{ next_fn{ next, next.id } };
        }

        sequence_t<face_t> adjacent_faces() const
        {
            struct next_fn
            {
                mutable halfedge_t next;
                const halfedge_id_type first_id;
                mutable bool done = false;

                maybe_t<face_t> operator()()
                {
                    while (true)
                    {
                        if (done)
                        {
                            return {};
                        }

                        auto current = next;
                        next = next.next_halfedge();
                        done = (next.id == first_id);
                        if (auto face = current.twin_halfedge().incident_face())
                        {
                            return face;
                        }
                    }
                }
            };

            halfedge_t next = m_self->halfedge(info().halfedge);
            return sequence_t<face_t>{ next_fn{ next, next.id } };
        }

        polygon_type as_polygon() const
        {
            return vertices().transform([](const vertex_t& v) { return v.location(); });
        }

        friend std::ostream& operator<<(std::ostream& os, const face_t& item) { return os << "F " << item.id; }

    private:
        const face_info_t& info() const { return m_self->get_face(id); }
    };

    struct halfedge_t
    {
        const dcel_t* m_self;
        halfedge_id_type id;

        maybe_t<face_t> incident_face() const
        {
            const auto& i = info();
            if (i.face == face_id_type{ -1 })
            {
                return {};
            }
            return face_t{ m_self, i.face };
        }

        halfedge_t twin_halfedge() const { return { m_self, info().twin_halfedge }; }

        halfedge_t next_halfedge() const { return { m_self, info().next_halfedge }; }

        halfedge_t prev_halfedge() const { return { m_self, info().prev_halfedge }; }

        vertex_t vertex_from() const { return { m_self, info().vertex_from }; }

        vertex_t vertex_to() const { return twin_halfedge().vertex_from(); }

        segment_type as_segment() const { return segment_type{ vertex_from().location(), vertex_to().location() }; }

        friend std::ostream& operator<<(std::ostream& os, const halfedge_t& item) { return os << item.info(); }

    private:
        const halfedge_info_t& info() const { return m_self->get_halfedge(id); }
    };

    // private:
    std::vector<vertex_info_t> m_vertices;
    std::vector<location_type> m_locations;
    std::vector<face_info_t> m_faces;
    std::vector<halfedge_info_t> m_halfedges;
    std::map<std::pair<vertex_id_type, vertex_id_type>, halfedge_id_type> m_edges;
    halfedge_id_type m_boundary_halfedge;

private:
    const location_type& get_location(vertex_id_type id) const { return m_locations.at(static_cast<std::size_t>(id)); }

    const vertex_info_t& get_vertex(vertex_id_type id) const { return m_vertices.at(static_cast<std::size_t>(id)); }
    vertex_info_t& get_vertex(vertex_id_type id) { return m_vertices.at(static_cast<std::size_t>(id)); }

    const halfedge_info_t& get_halfedge(halfedge_id_type id) const { return m_halfedges.at(static_cast<std::size_t>(id)); }
    halfedge_info_t& get_halfedge(halfedge_id_type id) { return m_halfedges.at(static_cast<std::size_t>(id)); }

    const face_info_t& get_face(face_id_type id) const { return m_faces.at(static_cast<std::size_t>(id)); }
    face_info_t& get_face(face_id_type id) { return m_faces.at(static_cast<std::size_t>(id)); }

    template <class IdType, class Type, class Func>
    static Type& new_item(std::vector<Type>& container, Func func)
    {
        auto id = IdType{ static_cast<int>(container.size()) };
        container.push_back(std::invoke(func, id));
        return container.back();
    }

    vertex_info_t& new_vertex()
    {
        return new_item<vertex_id_type>(m_vertices, [](vertex_id_type id) { return vertex_info_t{ id }; });
    }

    void set_location(vertex_id_type id, const location_type& location)
    {
        m_locations.resize(static_cast<std::size_t>(id + 1));
        m_locations.at(static_cast<std::size_t>(id)) = location;
    }

    face_info_t& new_face()
    {
        return new_item<face_id_type>(m_faces, [](face_id_type id) { return face_info_t{ id }; });
    }

    halfedge_id_type build_face(const std::vector<vertex_id_type>& vertices, face_info_t* face)
    {
        const auto buffer_begin = std::begin(vertices);
        const auto buffer_end = std::end(vertices);
        const auto buffer_size = static_cast<int>(std::distance(buffer_begin, buffer_end));

        const auto get = [=](int n) -> vertex_id_type
        {
            while (n < 0)
            {
                n += buffer_size;
            }

            return *(buffer_begin + (n % buffer_size));
        };

        for (int i = 0; i < buffer_size; ++i)
        {
            connect(get(i + 0), get(i + 1));
        }

        for (int i = 0; i < buffer_size; ++i)
        {
            halfedge_info_t& h0 = get_halfedge(*find_halfedge(get(i + 0), get(i + 1)));
            halfedge_info_t& h1 = get_halfedge(*find_halfedge(get(i + 1), get(i + 2)));

            if (vertex_info_t& v = get_vertex(get(i)); v.halfedge == halfedge_id_type{ -1 })
            {
                v.halfedge = h0.id;
            }

            h0.next_halfedge = h1.id;
            h1.prev_halfedge = h0.id;

            if (face)
            {
                if (i == 0)
                {
                    face->halfedge = h0.id;
                }

                h0.face = face->id;
            }
        }

        return m_edges.at(std::pair{ get(0), get(1) });
    }

    std::optional<halfedge_id_type> find_halfedge(vertex_id_type from, vertex_id_type to)
    {
        const auto key = std::pair{ from, to };
        if (const auto iter = m_edges.find(key); iter != m_edges.end())
        {
            return iter->second;
        }
        return {};
    }

    std::pair<halfedge_id_type, halfedge_id_type> connect(vertex_id_type from_vertex, vertex_id_type to_vertex)
    {
        const auto f = find_halfedge(from_vertex, to_vertex);
        const auto t = find_halfedge(to_vertex, from_vertex);

        if (f && t)
        {
            return { *f, *t };
        }

        auto [new_halfedges_begin, new_halfedges_end] = add_halfedges(2);

        halfedge_info_t& from_halfedge = new_halfedges_begin[0];
        halfedge_info_t& to_halfedge = new_halfedges_begin[1];

        from_halfedge.vertex_from = from_vertex;
        from_halfedge.twin_halfedge = to_halfedge.id;

        to_halfedge.vertex_from = to_vertex;
        to_halfedge.twin_halfedge = from_halfedge.id;

        m_edges.emplace(std::pair{ from_vertex, to_vertex }, from_halfedge.id);
        m_edges.emplace(std::pair{ to_vertex, from_vertex }, to_halfedge.id);

        return { from_halfedge.id, to_halfedge.id };
    }

    std::pair<std::vector<halfedge_info_t>::iterator, std::vector<halfedge_info_t>::iterator> add_halfedges(int count)
    {
        for (int i = 0; i < count; ++i)
        {
            new_item<halfedge_id_type>(m_halfedges, [](halfedge_id_type id) { return halfedge_info_t{ id }; });
        }
        return { m_halfedges.end() - count, m_halfedges.end() };
    }

    std::vector<vertex_id_type> hull() const
    {
        std::vector<vertex_id_type> result;

        std::unordered_map<vertex_id_type, vertex_id_type> outer_halfedges;
        for (halfedge_t h : halfedges())
        {
            if (!h.incident_face().has_value())
            {
                outer_halfedges.emplace(h.vertex_from().id, h.vertex_to().id);
            }
        }

        if (outer_halfedges.empty())
        {
            throw std::runtime_error{ "error on creating hull" };
        }

        vertex_id_type cur = std::begin(outer_halfedges)->first;

        while (result.size() != outer_halfedges.size())
        {
            vertex_id_type n = outer_halfedges.at(cur);
            result.push_back(n);
            cur = n;
        }

        return result;
    }
};

}  // namespace detail

using detail::dcel_t;

}  // namespace geometry

}  // namespace zx
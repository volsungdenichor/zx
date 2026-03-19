#pragma once

#include <set>
#include <unordered_map>
#include <zx/dcel.hpp>

namespace zx
{
namespace geometry
{

struct voronoi_fn
{
    template <class T>
    dcel_t<T> operator()(const dcel_t<T>& input) const
    {
        using dcel_vertex_id = typename dcel_t<T>::vertex_id_type;
        using vertex_t = typename dcel_t<T>::vertex_t;
        using halfedge_t = typename dcel_t<T>::halfedge_t;
        using face_t = typename dcel_t<T>::face_t;

        const std::vector<dcel_vertex_id> outer
            = input.outer_halfedges().transform([](const halfedge_t& halfedge) { return halfedge.vertex_from().id; });

        const auto is_outer_vertex
            = [&](const vertex_t& vertex) { return std::find(outer.begin(), outer.end(), vertex.id) != outer.end(); };

        std::unordered_map<typename dcel_t<T>::face_id_type, dcel_vertex_id> centers;

        dcel_t<T> result;

        const auto add_face = [&](const vertex_t& vertex)
        {
            std::vector<dcel_vertex_id> vertices;
            for (const face_t& face : vertex.incident_faces())
            {
                dcel_vertex_id v;
                if (auto it = centers.find(face.id); it != centers.end())
                {
                    v = it->second;
                }
                else
                {
                    v = result.add_vertex(get_center<T>(face));
                    centers[face.id] = v;
                }
                vertices.push_back(v);
            }

            if (vertices.size() >= 3)
            {
                std::reverse(vertices.begin(), vertices.end());
                result.add_face(vertices);
            }
        };

        for (const vertex_t& vertex : input.vertices().filter(std::not_fn(is_outer_vertex)))
        {
            add_face(vertex);
        }

        result.add_boundary();

        return result;
    }

    template <class T>
    static mat::triangle_t<T, 2> as_triangle(const typename dcel_t<T>::face_t& face)
    {
        const auto p = face.as_polygon();
        return { p.at(0), p.at(1), p.at(2) };
    }

    template <class T>
    static mat::vector_t<T, 2> get_center(const typename dcel_t<T>::face_t& face)
    {
        return mat::circumcenter(as_triangle<T>(face));
    }
};

struct triangulate_fn
{
    template <class T>
    dcel_t<T> operator()(std::vector<mat::vector_t<T, 2>> vertices) const
    {
        using triangle_info = std::array<std::size_t, 3>;
        using edge_info = std::array<std::size_t, 2>;
        using triangle_type = mat::triangle_t<T, 2>;

        const auto get_vertex = [&](std::size_t index) -> const mat::vector_t<T, 2>& { return vertices.at(index); };

        const auto get_triangle = [&](const triangle_info& t) -> triangle_type {
            return { get_vertex(t[0]), get_vertex(t[1]), get_vertex(t[2]) };
        };

        static const auto collinear = [](const triangle_type& t) -> bool
        { return std::abs(mat::orientation(t[0], t[1], t[2]) - T{ 0 }) < std::numeric_limits<T>::epsilon(); };

        const auto bounds = make_aabb(vertices);

        const auto max_dimension = std::max(bounds[0][1] - bounds[0][0], bounds[1][1] - bounds[1][0]);

        const auto c = mat::center(bounds);

        const T delta = T(20);

        std::vector<triangle_info> triangles;

        const auto s = vertices.size();

        vertices.push_back(c + mat::vector_t<T, 2>{ -delta * max_dimension, -max_dimension });
        vertices.push_back(c + mat::vector_t<T, 2>{ 0, +delta * max_dimension });
        vertices.push_back(c + mat::vector_t<T, 2>{ +delta * max_dimension, -max_dimension });

        const triangle_info super_triangle{ s + 0, s + 1, s + 2 };

        triangles.push_back(super_triangle);

        for (std::size_t i = 0; i < vertices.size(); ++i)
        {
            std::vector<triangle_info> invalid_triangles;
            std::vector<edge_info> edges;

            for (const triangle_info& triangle : triangles)
            {
                if (mat::contains(mat::circumcircle(get_triangle(triangle)), get_vertex(i)))
                {
                    invalid_triangles.push_back(triangle);

                    edges.push_back({ triangle.at(0), triangle.at(1) });
                    edges.push_back({ triangle.at(1), triangle.at(2) });
                    edges.push_back({ triangle.at(2), triangle.at(0) });
                }
            }

            remove(triangles, invalid_triangles);

            std::vector<edge_info> invalid_edges;

            for (const edge_info& edge1 : edges)
            {
                const auto v1 = get_vertices(edge1);
                for (const edge_info& edge2 : edges)
                {
                    if (&edge1 != &edge2 && v1 == get_vertices(edge2))
                    {
                        invalid_edges.push_back(edge1);
                    }
                }
            }

            remove(edges, invalid_edges);

            for (const edge_info& edge : edges)
            {
                triangle_info triangle{ edge[0], edge[1], i };

                if (!collinear(get_triangle(triangle)))
                {
                    triangles.push_back(triangle);
                }
            }
        }

        remove_erase_if(
            triangles,
            [&](const triangle_info& triangle)
            {
                const triangle_type t = get_triangle(triangle);
                return std::any_of(
                    super_triangle.begin(),
                    super_triangle.end(),
                    [&](std::size_t super_triangle_vertex) { return mat::contains(t, get_vertex(super_triangle_vertex)); });
            });

        dcel_t<T> result;

        std::map<std::size_t, typename dcel_t<T>::vertex_id_type> map;

        for (const triangle_info& triangle : triangles)
        {
            for (std::size_t v : triangle)
            {
                if (map.find(v) == map.end())
                {
                    map[v] = result.add_vertex(get_vertex(v));
                }
            }
        }

        for (triangle_info& triangle : triangles)
        {
            const mat::triangle_t<T, 2> t = get_triangle(triangle);
            if (mat::cross(t[1] - t[0], t[2] - t[0]) > 0.0)
            {
                std::reverse(triangle.begin(), triangle.end());
            }
            result.add_face({ map.at(triangle[0]), map.at(triangle[1]), map.at(triangle[2]) });
        }

        result.add_boundary();
        return result;
    }

    template <class T>
    static mat::box_shape_t<T, 2> make_aabb(const std::vector<mat::vector_t<T, 2>>& vertices)
    {
        if (vertices.empty())
        {
            return mat::box_shape_t<T, 2>{};
        }
        mat::box_shape_t<T, 2> result{
            mat::interval_t<T>{ vertices[0][0], vertices[0][0] },
            mat::interval_t<T>{ vertices[0][1], vertices[0][1] },
        };
        for (const mat::vector_t<T, 2>& vertex : vertices)
        {
            result = mat::box_shape_t<T, 2>{
                mat::interval_t<T>{ std::min(result[0][0], vertex[0]), std::max(result[0][1], vertex[0]) },
                mat::interval_t<T>{ std::min(result[1][0], vertex[1]), std::max(result[1][1], vertex[1]) },
            };
        }
        return result;
    }

    template <class T>
    void remove(std::vector<T>& lhs, const std::vector<T>& rhs) const
    {
        remove_erase_if(
            lhs,
            [&](const T& lhs_item)
            {
                const auto lhs_vertices = get_vertices(lhs_item);
                return std::any_of(
                    rhs.begin(), rhs.end(), [&](const T& rhs_item) { return lhs_vertices == get_vertices(rhs_item); });
            });
    }

    template <class T>
    std::set<std::size_t> get_vertices(const T& item) const
    {
        return std::set<std::size_t>{ item.begin(), item.end() };
    }

    template <class Container, class Pred>
    static void remove_erase_if(Container& container, Pred&& pred)
    {
        container.erase(std::remove_if(container.begin(), container.end(), std::forward<Pred>(pred)), container.end());
    }
};

constexpr inline auto voronoi = voronoi_fn{};
constexpr inline auto triangulate = triangulate_fn{};

}  // namespace geometry
}  // namespace zx

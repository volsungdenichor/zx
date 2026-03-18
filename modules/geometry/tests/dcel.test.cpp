#include <gmock/gmock.h>

#include <zx/dcel.hpp>

namespace zx
{

template <class T, class NextFn>
void PrintTo(const sequence_t<T, NextFn>& seq, std::ostream* os)
{
    *os << "sequence{"
        << "\n";
    auto it = seq.begin();
    auto e = seq.end();
    std::size_t index = 0;
    while (it != e)
    {
        *os << "[" << index << "] " << *it << "\n";
        ++it;
        ++index;
    }
    *os << "]"
        << "\n";
}
}  // namespace zx

constexpr auto id_is = [](auto&& matcher)
{
    return testing::ResultOf(
        "id", [](const auto& item) { return item.id; }, std::forward<decltype(matcher)>(matcher));
};

constexpr auto location_is = [](auto&& matcher)
{
    return testing::ResultOf(
        "location", [](const auto& item) { return item.location(); }, std::forward<decltype(matcher)>(matcher));
};

constexpr auto out_halfedges_are = [](auto&& matcher)
{
    return testing::ResultOf(
        "out_halfedges", [](const auto& item) { return item.out_halfedges(); }, std::forward<decltype(matcher)>(matcher));
};

constexpr auto in_halfedges_are = [](auto&& matcher)
{
    return testing::ResultOf(
        "in_halfedges", [](const auto& item) { return item.in_halfedges(); }, std::forward<decltype(matcher)>(matcher));
};

constexpr auto incident_faces_are = [](auto&& matcher)
{
    return testing::ResultOf(
        "incident_faces", [](const auto& item) { return item.incident_faces(); }, std::forward<decltype(matcher)>(matcher));
};

constexpr auto halfedge_is = [](auto&& matcher)
{
    return testing::ResultOf(
        "halfedge", [](const auto& item) { return item.halfedge(); }, std::forward<decltype(matcher)>(matcher));
};

constexpr auto halfedges_are = [](auto&& matcher)
{
    return testing::ResultOf(
        "halfedges", [](const auto& item) { return item.halfedges(); }, std::forward<decltype(matcher)>(matcher));
};

constexpr auto vertices_are = [](auto&& matcher)
{
    return testing::ResultOf(
        "vertices", [](const auto& item) { return item.vertices(); }, std::forward<decltype(matcher)>(matcher));
};

constexpr auto as_polygon_is = [](auto&& matcher)
{
    return testing::ResultOf(
        "as_polygon", [](const auto& item) { return item.as_polygon(); }, std::forward<decltype(matcher)>(matcher));
};

constexpr auto twin_halfedge_is = [](auto&& matcher)
{
    return testing::ResultOf(
        "twin_halfedge", [](const auto& item) { return item.twin_halfedge(); }, std::forward<decltype(matcher)>(matcher));
};

constexpr auto next_halfedge_is = [](auto&& matcher)
{
    return testing::ResultOf(
        "next_halfedge", [](const auto& item) { return item.next_halfedge(); }, std::forward<decltype(matcher)>(matcher));
};

constexpr auto prev_halfedge_is = [](auto&& matcher)
{
    return testing::ResultOf(
        "prev_halfedge", [](const auto& item) { return item.prev_halfedge(); }, std::forward<decltype(matcher)>(matcher));
};

constexpr auto vertex_from_is = [](auto&& matcher)
{
    return testing::ResultOf(
        "vertex_from", [](const auto& item) { return item.vertex_from(); }, std::forward<decltype(matcher)>(matcher));
};

constexpr auto vertex_to_is = [](auto&& matcher)
{
    return testing::ResultOf(
        "vertex_to", [](const auto& item) { return item.vertex_to(); }, std::forward<decltype(matcher)>(matcher));
};

constexpr auto incident_face_is = [](auto&& matcher)
{
    return testing::ResultOf(
        "incident_face", [](const auto& item) { return item.incident_face(); }, std::forward<decltype(matcher)>(matcher));
};

TEST(dcel, access)
{
    using D = zx::geometry::dcel_t<int>;
    D dcel = {};

    dcel.m_locations.push_back(zx::mat::vector(648, 112));
    dcel.m_locations.push_back(zx::mat::vector(385, 147));
    dcel.m_locations.push_back(zx::mat::vector(459, 303));
    dcel.m_locations.push_back(zx::mat::vector(656, 330));

    dcel.m_vertices.push_back({ 0, 0 });
    dcel.m_vertices.push_back({ 1, 2 });
    dcel.m_vertices.push_back({ 2, 4 });
    dcel.m_vertices.push_back({ 3, 6 });

    const auto v = std::vector<D::vertex_id_type>{
        dcel.m_vertices[0].id,
        dcel.m_vertices[1].id,
        dcel.m_vertices[2].id,
        dcel.m_vertices[3].id,
    };

    dcel.m_faces.push_back({ 0, 0 });
    dcel.m_faces.push_back({ 1, 6 });

    dcel.m_halfedges.push_back({ 0, v[0], 1, 2, 4, 0 });
    dcel.m_halfedges.push_back({ 1, v[1], 0, 7, 3, -1 });
    dcel.m_halfedges.push_back({ 2, v[1], 3, 4, 0, 0 });
    dcel.m_halfedges.push_back({ 3, v[2], 2, 1, 9, -1 });
    dcel.m_halfedges.push_back({ 4, v[2], 5, 0, 2, 0 });
    dcel.m_halfedges.push_back({ 5, v[0], 4, 8, 6, 1 });
    dcel.m_halfedges.push_back({ 6, v[3], 7, 5, 8, 1 });
    dcel.m_halfedges.push_back({ 7, v[0], 6, 9, 1, -1 });
    dcel.m_halfedges.push_back({ 8, v[2], 9, 6, 5, 1 });
    dcel.m_halfedges.push_back({ 9, v[3], 8, 3, 7, -1 });

    dcel.m_edges = {
        { { 0, 1 }, 0 }, { { 0, 2 }, 5 }, { { 0, 3 }, 7 }, { { 1, 0 }, 1 }, { { 1, 2 }, 2 },
        { { 2, 0 }, 4 }, { { 2, 1 }, 3 }, { { 2, 3 }, 8 }, { { 3, 0 }, 6 }, { { 3, 2 }, 9 },
    };

    dcel.m_boundary_halfedge = 3;

    const auto vertices
        = std::vector<testing::Matcher<D::vertex_t>>{ testing::AllOf(
                                                          id_is(0),
                                                          location_is(zx::mat::vector(648, 112)),
                                                          out_halfedges_are(testing::ElementsAre(id_is(0), id_is(7), id_is(5))),
                                                          in_halfedges_are(testing::ElementsAre(id_is(1), id_is(6), id_is(4))),
                                                          incident_faces_are(testing::ElementsAre(id_is(1), id_is(0)))),
                                                      testing::AllOf(
                                                          id_is(1),
                                                          location_is(zx::mat::vector(385, 147)),
                                                          out_halfedges_are(testing::ElementsAre(id_is(2), id_is(1))),
                                                          in_halfedges_are(testing::ElementsAre(id_is(3), id_is(0))),
                                                          incident_faces_are(testing::ElementsAre(id_is(0)))),
                                                      testing::AllOf(
                                                          id_is(2),
                                                          location_is(zx::mat::vector(459, 303)),
                                                          out_halfedges_are(testing::ElementsAre(id_is(4), id_is(8), id_is(3))),
                                                          in_halfedges_are(testing::ElementsAre(id_is(5), id_is(9), id_is(2))),
                                                          incident_faces_are(testing::ElementsAre(id_is(1), id_is(0)))),
                                                      testing::AllOf(
                                                          id_is(3),
                                                          location_is(zx::mat::vector(656, 330)),
                                                          out_halfedges_are(testing::ElementsAre(id_is(6), id_is(9))),
                                                          in_halfedges_are(testing::ElementsAre(id_is(7), id_is(8))),
                                                          incident_faces_are(testing::ElementsAre(id_is(1)))) };

    const auto faces = std::vector<testing::Matcher<D::face_t>>{
        testing::AllOf(
            id_is(0),
            halfedge_is(id_is(0)),
            halfedges_are(testing::ElementsAre(id_is(0), id_is(2), id_is(4))),
            vertices_are(testing::ElementsAre(id_is(0), id_is(1), id_is(2))),
            as_polygon_is(zx::mat::polygon_t<int, 2>{
                zx::mat::vector(648, 112), zx::mat::vector(385, 147), zx::mat::vector(459, 303) })),
        testing::AllOf(
            id_is(1),
            halfedge_is(id_is(6)),
            halfedges_are(testing::ElementsAre(id_is(6), id_is(5), id_is(8))),
            vertices_are(testing::ElementsAre(id_is(3), id_is(0), id_is(2))),
            as_polygon_is(zx::mat::polygon_t<int, 2>{
                zx::mat::vector(656, 330), zx::mat::vector(648, 112), zx::mat::vector(459, 303) }))
    };

    const auto halfedges
        = std::vector<testing::Matcher<D::halfedge_t>>{ testing::AllOf(
                                                            id_is(0),
                                                            twin_halfedge_is(id_is(1)),
                                                            next_halfedge_is(id_is(2)),
                                                            prev_halfedge_is(id_is(4)),
                                                            incident_face_is(testing::Optional(id_is(0))),
                                                            vertex_from_is(id_is(0)),
                                                            vertex_to_is(id_is(1))),
                                                        testing::AllOf(
                                                            id_is(1),
                                                            twin_halfedge_is(id_is(0)),
                                                            next_halfedge_is(id_is(7)),
                                                            prev_halfedge_is(id_is(3)),
                                                            incident_face_is(testing::Eq(zx::none)),
                                                            vertex_from_is(id_is(1)),
                                                            vertex_to_is(id_is(0))),
                                                        testing::AllOf(
                                                            id_is(2),
                                                            twin_halfedge_is(id_is(3)),
                                                            next_halfedge_is(id_is(4)),
                                                            prev_halfedge_is(id_is(0)),
                                                            incident_face_is(testing::Optional(id_is(0))),
                                                            vertex_from_is(id_is(1)),
                                                            vertex_to_is(id_is(2))),
                                                        testing::AllOf(
                                                            id_is(3),
                                                            twin_halfedge_is(id_is(2)),
                                                            next_halfedge_is(id_is(1)),
                                                            prev_halfedge_is(id_is(9)),
                                                            incident_face_is(testing::Eq(zx::none)),
                                                            vertex_from_is(id_is(2)),
                                                            vertex_to_is(id_is(1))),
                                                        testing::AllOf(
                                                            id_is(4),
                                                            twin_halfedge_is(id_is(5)),
                                                            next_halfedge_is(id_is(0)),
                                                            prev_halfedge_is(id_is(2)),
                                                            incident_face_is(testing::Optional(id_is(0))),
                                                            vertex_from_is(id_is(2)),
                                                            vertex_to_is(id_is(0))),
                                                        testing::AllOf(
                                                            id_is(5),
                                                            twin_halfedge_is(id_is(4)),
                                                            next_halfedge_is(id_is(8)),
                                                            prev_halfedge_is(id_is(6)),
                                                            incident_face_is(testing::Optional(id_is(1))),
                                                            vertex_from_is(id_is(0)),
                                                            vertex_to_is(id_is(2))),
                                                        testing::AllOf(
                                                            id_is(6),
                                                            twin_halfedge_is(id_is(7)),
                                                            next_halfedge_is(id_is(5)),
                                                            prev_halfedge_is(id_is(8)),
                                                            incident_face_is(testing::Optional(id_is(1))),
                                                            vertex_from_is(id_is(3)),
                                                            vertex_to_is(id_is(0))),
                                                        testing::AllOf(
                                                            id_is(7),
                                                            twin_halfedge_is(id_is(6)),
                                                            next_halfedge_is(id_is(9)),
                                                            prev_halfedge_is(id_is(1)),
                                                            incident_face_is(testing::Eq(zx::none)),
                                                            vertex_from_is(id_is(0)),
                                                            vertex_to_is(id_is(3))),
                                                        testing::AllOf(
                                                            id_is(8),
                                                            twin_halfedge_is(id_is(9)),
                                                            next_halfedge_is(id_is(6)),
                                                            prev_halfedge_is(id_is(5)),
                                                            incident_face_is(testing::Optional(id_is(1))),
                                                            vertex_from_is(id_is(2)),
                                                            vertex_to_is(id_is(3))),
                                                        testing::AllOf(
                                                            id_is(9),
                                                            twin_halfedge_is(id_is(8)),
                                                            next_halfedge_is(id_is(3)),
                                                            prev_halfedge_is(id_is(7)),
                                                            incident_face_is(testing::Eq(zx::none)),
                                                            vertex_from_is(id_is(3)),
                                                            vertex_to_is(id_is(2))) };

    EXPECT_THAT(dcel.vertices(), testing::ElementsAreArray(vertices));
    EXPECT_THAT(dcel.vertex(0), vertices[0]);
    EXPECT_THAT(dcel.vertex(1), vertices[1]);
    EXPECT_THAT(dcel.vertex(2), vertices[2]);
    EXPECT_THAT(dcel.vertex(3), vertices[3]);

    EXPECT_THAT(dcel.faces(), testing::ElementsAreArray(faces));
    EXPECT_THAT(dcel.face(0), faces[0]);
    EXPECT_THAT(dcel.face(1), faces[1]);

    EXPECT_THAT(dcel.halfedges(), testing::ElementsAreArray(halfedges));
    EXPECT_THAT(dcel.halfedge(0), halfedges[0]);
    EXPECT_THAT(dcel.halfedge(1), halfedges[1]);
    EXPECT_THAT(dcel.halfedge(2), halfedges[2]);
    EXPECT_THAT(dcel.halfedge(3), halfedges[3]);
    EXPECT_THAT(dcel.halfedge(4), halfedges[4]);
    EXPECT_THAT(dcel.halfedge(5), halfedges[5]);
    EXPECT_THAT(dcel.halfedge(6), halfedges[6]);
    EXPECT_THAT(dcel.halfedge(7), halfedges[7]);
    EXPECT_THAT(dcel.halfedge(8), halfedges[8]);
    EXPECT_THAT(dcel.halfedge(9), halfedges[9]);

    EXPECT_THAT(dcel.outer_halfedges(), testing::ElementsAre(id_is(3), id_is(1), id_is(7), id_is(9)));
}

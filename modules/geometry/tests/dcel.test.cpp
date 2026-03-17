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

constexpr auto IdIs = [](auto&& matcher)
{
    return testing::ResultOf(
        "id", [](const auto& item) { return item.id; }, std::forward<decltype(matcher)>(matcher));
};

constexpr auto LocationIs = [](auto&& matcher)
{
    return testing::ResultOf(
        "location", [](const auto& item) { return item.location(); }, std::forward<decltype(matcher)>(matcher));
};

constexpr auto OutHalfedgesAre = [](auto&& matcher)
{
    return testing::ResultOf(
        "out_halfedges", [](const auto& item) { return item.out_halfedges(); }, std::forward<decltype(matcher)>(matcher));
};

constexpr auto InHalfedgesAre = [](auto&& matcher)
{
    return testing::ResultOf(
        "in_halfedges", [](const auto& item) { return item.in_halfedges(); }, std::forward<decltype(matcher)>(matcher));
};

constexpr auto IncidentFacesAre = [](auto&& matcher)
{
    return testing::ResultOf(
        "incident_faces", [](const auto& item) { return item.incident_faces(); }, std::forward<decltype(matcher)>(matcher));
};

constexpr auto HalfedgeIs = [](auto&& matcher)
{
    return testing::ResultOf(
        "halfedge", [](const auto& item) { return item.halfedge(); }, std::forward<decltype(matcher)>(matcher));
};

constexpr auto HalfedgesAre = [](auto&& matcher)
{
    return testing::ResultOf(
        "halfedges", [](const auto& item) { return item.halfedges(); }, std::forward<decltype(matcher)>(matcher));
};

constexpr auto VerticesAre = [](auto&& matcher)
{
    return testing::ResultOf(
        "vertices", [](const auto& item) { return item.vertices(); }, std::forward<decltype(matcher)>(matcher));
};

constexpr auto AsPolygonIs = [](auto&& matcher)
{
    return testing::ResultOf(
        "as_polygon", [](const auto& item) { return item.as_polygon(); }, std::forward<decltype(matcher)>(matcher));
};

constexpr auto TwinHalfedgeIs = [](auto&& matcher)
{
    return testing::ResultOf(
        "twin_halfedge", [](const auto& item) { return item.twin_halfedge(); }, std::forward<decltype(matcher)>(matcher));
};

constexpr auto NextHalfedgeIs = [](auto&& matcher)
{
    return testing::ResultOf(
        "next_halfedge", [](const auto& item) { return item.next_halfedge(); }, std::forward<decltype(matcher)>(matcher));
};

constexpr auto PrevHalfedgeIs = [](auto&& matcher)
{
    return testing::ResultOf(
        "prev_halfedge", [](const auto& item) { return item.prev_halfedge(); }, std::forward<decltype(matcher)>(matcher));
};

constexpr auto VertexFromIs = [](auto&& matcher)
{
    return testing::ResultOf(
        "vertex_from", [](const auto& item) { return item.vertex_from(); }, std::forward<decltype(matcher)>(matcher));
};

constexpr auto VertexToIs = [](auto&& matcher)
{
    return testing::ResultOf(
        "vertex_to", [](const auto& item) { return item.vertex_to(); }, std::forward<decltype(matcher)>(matcher));
};

constexpr auto IncidentFaceIs = [](auto&& matcher)
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
                                                          IdIs(0),
                                                          LocationIs(zx::mat::vector(648, 112)),
                                                          OutHalfedgesAre(testing::ElementsAre(IdIs(0), IdIs(7), IdIs(5))),
                                                          InHalfedgesAre(testing::ElementsAre(IdIs(1), IdIs(6), IdIs(4))),
                                                          IncidentFacesAre(testing::ElementsAre(IdIs(1), IdIs(0)))),
                                                      testing::AllOf(
                                                          IdIs(1),
                                                          LocationIs(zx::mat::vector(385, 147)),
                                                          OutHalfedgesAre(testing::ElementsAre(IdIs(2), IdIs(1))),
                                                          InHalfedgesAre(testing::ElementsAre(IdIs(3), IdIs(0))),
                                                          IncidentFacesAre(testing::ElementsAre(IdIs(0)))),
                                                      testing::AllOf(
                                                          IdIs(2),
                                                          LocationIs(zx::mat::vector(459, 303)),
                                                          OutHalfedgesAre(testing::ElementsAre(IdIs(4), IdIs(8), IdIs(3))),
                                                          InHalfedgesAre(testing::ElementsAre(IdIs(5), IdIs(9), IdIs(2))),
                                                          IncidentFacesAre(testing::ElementsAre(IdIs(1), IdIs(0)))),
                                                      testing::AllOf(
                                                          IdIs(3),
                                                          LocationIs(zx::mat::vector(656, 330)),
                                                          OutHalfedgesAre(testing::ElementsAre(IdIs(6), IdIs(9))),
                                                          InHalfedgesAre(testing::ElementsAre(IdIs(7), IdIs(8))),
                                                          IncidentFacesAre(testing::ElementsAre(IdIs(1)))) };

    const auto faces = std::vector<testing::Matcher<D::face_t>>{
        testing::AllOf(
            IdIs(0),
            HalfedgeIs(IdIs(0)),
            HalfedgesAre(testing::ElementsAre(IdIs(0), IdIs(2), IdIs(4))),
            VerticesAre(testing::ElementsAre(IdIs(0), IdIs(1), IdIs(2))),
            AsPolygonIs(zx::mat::polygon_t<int, 2>{
                zx::mat::vector(648, 112), zx::mat::vector(385, 147), zx::mat::vector(459, 303) })),
        testing::AllOf(
            IdIs(1),
            HalfedgeIs(IdIs(6)),
            HalfedgesAre(testing::ElementsAre(IdIs(6), IdIs(5), IdIs(8))),
            VerticesAre(testing::ElementsAre(IdIs(3), IdIs(0), IdIs(2))),
            AsPolygonIs(zx::mat::polygon_t<int, 2>{
                zx::mat::vector(656, 330), zx::mat::vector(648, 112), zx::mat::vector(459, 303) }))
    };

    const auto halfedges
        = std::vector<testing::Matcher<D::halfedge_t>>{ testing::AllOf(
                                                            IdIs(0),
                                                            TwinHalfedgeIs(IdIs(1)),
                                                            NextHalfedgeIs(IdIs(2)),
                                                            PrevHalfedgeIs(IdIs(4)),
                                                            IncidentFaceIs(testing::Optional(IdIs(0))),
                                                            VertexFromIs(IdIs(0)),
                                                            VertexToIs(IdIs(1))),
                                                        testing::AllOf(
                                                            IdIs(1),
                                                            TwinHalfedgeIs(IdIs(0)),
                                                            NextHalfedgeIs(IdIs(7)),
                                                            PrevHalfedgeIs(IdIs(3)),
                                                            IncidentFaceIs(testing::Eq(zx::none)),
                                                            VertexFromIs(IdIs(1)),
                                                            VertexToIs(IdIs(0))),
                                                        testing::AllOf(
                                                            IdIs(2),
                                                            TwinHalfedgeIs(IdIs(3)),
                                                            NextHalfedgeIs(IdIs(4)),
                                                            PrevHalfedgeIs(IdIs(0)),
                                                            IncidentFaceIs(testing::Optional(IdIs(0))),
                                                            VertexFromIs(IdIs(1)),
                                                            VertexToIs(IdIs(2))),
                                                        testing::AllOf(
                                                            IdIs(3),
                                                            TwinHalfedgeIs(IdIs(2)),
                                                            NextHalfedgeIs(IdIs(1)),
                                                            PrevHalfedgeIs(IdIs(9)),
                                                            IncidentFaceIs(testing::Eq(zx::none)),
                                                            VertexFromIs(IdIs(2)),
                                                            VertexToIs(IdIs(1))),
                                                        testing::AllOf(
                                                            IdIs(4),
                                                            TwinHalfedgeIs(IdIs(5)),
                                                            NextHalfedgeIs(IdIs(0)),
                                                            PrevHalfedgeIs(IdIs(2)),
                                                            IncidentFaceIs(testing::Optional(IdIs(0))),
                                                            VertexFromIs(IdIs(2)),
                                                            VertexToIs(IdIs(0))),
                                                        testing::AllOf(
                                                            IdIs(5),
                                                            TwinHalfedgeIs(IdIs(4)),
                                                            NextHalfedgeIs(IdIs(8)),
                                                            PrevHalfedgeIs(IdIs(6)),
                                                            IncidentFaceIs(testing::Optional(IdIs(1))),
                                                            VertexFromIs(IdIs(0)),
                                                            VertexToIs(IdIs(2))),
                                                        testing::AllOf(
                                                            IdIs(6),
                                                            TwinHalfedgeIs(IdIs(7)),
                                                            NextHalfedgeIs(IdIs(5)),
                                                            PrevHalfedgeIs(IdIs(8)),
                                                            IncidentFaceIs(testing::Optional(IdIs(1))),
                                                            VertexFromIs(IdIs(3)),
                                                            VertexToIs(IdIs(0))),
                                                        testing::AllOf(
                                                            IdIs(7),
                                                            TwinHalfedgeIs(IdIs(6)),
                                                            NextHalfedgeIs(IdIs(9)),
                                                            PrevHalfedgeIs(IdIs(1)),
                                                            IncidentFaceIs(testing::Eq(zx::none)),
                                                            VertexFromIs(IdIs(0)),
                                                            VertexToIs(IdIs(3))),
                                                        testing::AllOf(
                                                            IdIs(8),
                                                            TwinHalfedgeIs(IdIs(9)),
                                                            NextHalfedgeIs(IdIs(6)),
                                                            PrevHalfedgeIs(IdIs(5)),
                                                            IncidentFaceIs(testing::Optional(IdIs(1))),
                                                            VertexFromIs(IdIs(2)),
                                                            VertexToIs(IdIs(3))),
                                                        testing::AllOf(
                                                            IdIs(9),
                                                            TwinHalfedgeIs(IdIs(8)),
                                                            NextHalfedgeIs(IdIs(3)),
                                                            PrevHalfedgeIs(IdIs(7)),
                                                            IncidentFaceIs(testing::Eq(zx::none)),
                                                            VertexFromIs(IdIs(3)),
                                                            VertexToIs(IdIs(2))) };

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

    EXPECT_THAT(dcel.outer_halfedges(), testing::ElementsAre(IdIs(3), IdIs(1), IdIs(7), IdIs(9)));
}

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
        "Id", [](const auto& item) { return item.id; }, std::forward<decltype(matcher)>(matcher));
};

cons

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

    const auto vertices = std::vector<testing::Matcher<D::vertex_t>>{
        testing::AllOf(
            IdIs(0),
            testing::Property("location", &D::vertex_t::location, zx::mat::vector(648, 112)),
            testing::Property("out_halfedges", &D::vertex_t::out_halfedges, testing::ElementsAre(IdIs(0), IdIs(7), IdIs(5))),
            testing::Property("in_halfedges", &D::vertex_t::in_halfedges, testing::ElementsAre(IdIs(1), IdIs(6), IdIs(4))),
            testing::Property("incident_faces", &D::vertex_t::incident_faces, testing::ElementsAre(IdIs(1), IdIs(0)))),
        testing::AllOf(
            IdIs(1),
            testing::Property("location", &D::vertex_t::location, zx::mat::vector(385, 147)),
            testing::Property("out_halfedges", &D::vertex_t::out_halfedges, testing::ElementsAre(IdIs(2), IdIs(1))),
            testing::Property("in_halfedges", &D::vertex_t::in_halfedges, testing::ElementsAre(IdIs(3), IdIs(0))),
            testing::Property("incident_faces", &D::vertex_t::incident_faces, testing::ElementsAre(IdIs(0)))),
        testing::AllOf(
            IdIs(2),
            testing::Property("location", &D::vertex_t::location, zx::mat::vector(459, 303)),
            testing::Property("out_halfedges", &D::vertex_t::out_halfedges, testing::ElementsAre(IdIs(4), IdIs(8), IdIs(3))),
            testing::Property("in_halfedges", &D::vertex_t::in_halfedges, testing::ElementsAre(IdIs(5), IdIs(9), IdIs(2))),
            testing::Property("incident_faces", &D::vertex_t::incident_faces, testing::ElementsAre(IdIs(1), IdIs(0)))),
        testing::AllOf(
            IdIs(3),
            testing::Property("location", &D::vertex_t::location, zx::mat::vector(656, 330)),
            testing::Property("out_halfedges", &D::vertex_t::out_halfedges, testing::ElementsAre(IdIs(6), IdIs(9))),
            testing::Property("in_halfedges", &D::vertex_t::in_halfedges, testing::ElementsAre(IdIs(7), IdIs(8))),
            testing::Property("incident_faces", &D::vertex_t::incident_faces, testing::ElementsAre(IdIs(1))))
    };

    const auto faces = std::vector<testing::Matcher<D::face_t>>{
        testing::AllOf(
            IdIs(0),
            testing::Property("halfedge", &D::face_t::halfedge, IdIs(0)),
            testing::Property("halfedges", &D::face_t::halfedges, testing::ElementsAre(IdIs(0), IdIs(2), IdIs(4))),
            testing::Property("vertices", &D::face_t::vertices, testing::ElementsAre(IdIs(0), IdIs(1), IdIs(2))),
            testing::Property(
                "as_polygon",
                &D::face_t::as_polygon,
                zx::mat::polygon_t<int, 2>{
                    zx::mat::vector(648, 112), zx::mat::vector(385, 147), zx::mat::vector(459, 303) })),
        testing::AllOf(
            IdIs(1),
            testing::Property("halfedge", &D::face_t::halfedge, IdIs(6)),
            testing::Property("halfedges", &D::face_t::halfedges, testing::ElementsAre(IdIs(6), IdIs(5), IdIs(8))),
            testing::Property("vertices", &D::face_t::vertices, testing::ElementsAre(IdIs(3), IdIs(0), IdIs(2))),
            testing::Property(
                "as_polygon",
                &D::face_t::as_polygon,
                zx::mat::polygon_t<int, 2>{
                    zx::mat::vector(656, 330), zx::mat::vector(648, 112), zx::mat::vector(459, 303) }))
    };

    const auto halfedges = std::vector<testing::Matcher<D::halfedge_t>>{
        testing::AllOf(
            IdIs(0),
            testing::Property("twin_halfedge", &D::halfedge_t::twin_halfedge, IdIs(1)),
            testing::Property("next_halfedge", &D::halfedge_t::next_halfedge, IdIs(2)),
            testing::Property("prev_halfedge", &D::halfedge_t::prev_halfedge, IdIs(4)),
            testing::Property("incident_face", &D::halfedge_t::incident_face, testing::Optional(IdIs(0))),
            testing::Property("vertex_from", &D::halfedge_t::vertex_from, IdIs(0)),
            testing::Property("vertex_to", &D::halfedge_t::vertex_to, IdIs(1))),
        testing::AllOf(
            IdIs(1),
            testing::Property("twin_halfedge", &D::halfedge_t::twin_halfedge, IdIs(0)),
            testing::Property("next_halfedge", &D::halfedge_t::next_halfedge, IdIs(7)),
            testing::Property("prev_halfedge", &D::halfedge_t::prev_halfedge, IdIs(3)),
            testing::Property("incident_face", &D::halfedge_t::incident_face, testing::Eq(zx::none)),
            testing::Property("vertex_from", &D::halfedge_t::vertex_from, IdIs(1)),
            testing::Property("vertex_to", &D::halfedge_t::vertex_to, IdIs(0))),
        testing::AllOf(
            IdIs(2),
            testing::Property("twin_halfedge", &D::halfedge_t::twin_halfedge, IdIs(3)),
            testing::Property("next_halfedge", &D::halfedge_t::next_halfedge, IdIs(4)),
            testing::Property("prev_halfedge", &D::halfedge_t::prev_halfedge, IdIs(0)),
            testing::Property("incident_face", &D::halfedge_t::incident_face, testing::Optional(IdIs(0))),
            testing::Property("vertex_from", &D::halfedge_t::vertex_from, IdIs(1)),
            testing::Property("vertex_to", &D::halfedge_t::vertex_to, IdIs(2))),
        testing::AllOf(
            IdIs(3),
            testing::Property("twin_halfedge", &D::halfedge_t::twin_halfedge, IdIs(2)),
            testing::Property("next_halfedge", &D::halfedge_t::next_halfedge, IdIs(1)),
            testing::Property("prev_halfedge", &D::halfedge_t::prev_halfedge, IdIs(9)),
            testing::Property("incident_face", &D::halfedge_t::incident_face, testing::Eq(zx::none)),
            testing::Property("vertex_from", &D::halfedge_t::vertex_from, IdIs(2)),
            testing::Property("vertex_to", &D::halfedge_t::vertex_to, IdIs(1))),
        testing::AllOf(
            IdIs(4),
            testing::Property("twin_halfedge", &D::halfedge_t::twin_halfedge, IdIs(5)),
            testing::Property("next_halfedge", &D::halfedge_t::next_halfedge, IdIs(0)),
            testing::Property("prev_halfedge", &D::halfedge_t::prev_halfedge, IdIs(2)),
            testing::Property("incident_face", &D::halfedge_t::incident_face, testing::Optional(IdIs(0))),
            testing::Property("vertex_from", &D::halfedge_t::vertex_from, IdIs(2)),
            testing::Property("vertex_to", &D::halfedge_t::vertex_to, IdIs(0))),
        testing::AllOf(
            IdIs(5),
            testing::Property("twin_halfedge", &D::halfedge_t::twin_halfedge, IdIs(4)),
            testing::Property("next_halfedge", &D::halfedge_t::next_halfedge, IdIs(8)),
            testing::Property("prev_halfedge", &D::halfedge_t::prev_halfedge, IdIs(6)),
            testing::Property("incident_face", &D::halfedge_t::incident_face, testing::Optional(IdIs(1))),
            testing::Property("vertex_from", &D::halfedge_t::vertex_from, IdIs(0)),
            testing::Property("vertex_to", &D::halfedge_t::vertex_to, IdIs(2))),
        testing::AllOf(
            IdIs(6),
            testing::Property("twin_halfedge", &D::halfedge_t::twin_halfedge, IdIs(7)),
            testing::Property("next_halfedge", &D::halfedge_t::next_halfedge, IdIs(5)),
            testing::Property("prev_halfedge", &D::halfedge_t::prev_halfedge, IdIs(8)),
            testing::Property("incident_face", &D::halfedge_t::incident_face, testing::Optional(IdIs(1))),
            testing::Property("vertex_from", &D::halfedge_t::vertex_from, IdIs(3)),
            testing::Property("vertex_to", &D::halfedge_t::vertex_to, IdIs(0))),
        testing::AllOf(
            IdIs(7),
            testing::Property("twin_halfedge", &D::halfedge_t::twin_halfedge, IdIs(6)),
            testing::Property("next_halfedge", &D::halfedge_t::next_halfedge, IdIs(9)),
            testing::Property("prev_halfedge", &D::halfedge_t::prev_halfedge, IdIs(1)),
            testing::Property("incident_face", &D::halfedge_t::incident_face, testing::Eq(zx::none)),
            testing::Property("vertex_from", &D::halfedge_t::vertex_from, IdIs(0)),
            testing::Property("vertex_to", &D::halfedge_t::vertex_to, IdIs(3))),
        testing::AllOf(
            IdIs(8),
            testing::Property("twin_halfedge", &D::halfedge_t::twin_halfedge, IdIs(9)),
            testing::Property("next_halfedge", &D::halfedge_t::next_halfedge, IdIs(6)),
            testing::Property("prev_halfedge", &D::halfedge_t::prev_halfedge, IdIs(5)),
            testing::Property("incident_face", &D::halfedge_t::incident_face, testing::Optional(IdIs(1))),
            testing::Property("vertex_from", &D::halfedge_t::vertex_from, IdIs(2)),
            testing::Property("vertex_to", &D::halfedge_t::vertex_to, IdIs(3))),
        testing::AllOf(
            IdIs(9),
            testing::Property("twin_halfedge", &D::halfedge_t::twin_halfedge, IdIs(8)),
            testing::Property("next_halfedge", &D::halfedge_t::next_halfedge, IdIs(3)),
            testing::Property("prev_halfedge", &D::halfedge_t::prev_halfedge, IdIs(7)),
            testing::Property("incident_face", &D::halfedge_t::incident_face, testing::Eq(zx::none)),
            testing::Property("vertex_from", &D::halfedge_t::vertex_from, IdIs(3)),
            testing::Property("vertex_to", &D::halfedge_t::vertex_to, IdIs(2)))
    };

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

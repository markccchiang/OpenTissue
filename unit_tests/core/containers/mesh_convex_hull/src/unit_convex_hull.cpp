//
// OpenTissue, A toolbox for physical based simulation and animation.
// Copyright (C) 2007 Department of Computer Science, University of Copenhagen
//
#include <OpenTissue/configuration.h>

#include <OpenTissue/core/math/math_basic_types.h>
#include <OpenTissue/core/containers/mesh/mesh.h>

#define BOOST_AUTO_TEST_MAIN
#include <OpenTissue/utility/utility_push_boost_filter.h>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>
#include <boost/test/tools/floating_point_comparison.hpp>
#include <boost/test/test_tools.hpp>
#include <OpenTissue/utility/utility_pop_boost_filter.h>

#include <cmath>
#include <vector>

//
// convex_hull() is the only place OpenTissue drives Qhull's hull code, and until now nothing
// ran it: the polymesh and trimesh tests that mention it are compile-only -- they take the
// address of a function and never call it. So a change to how Qhull is called could not be
// told apart from no change at all.
//
// These cases execute it. The cube is the useful shape to ask for: every corner has to appear
// in the hull, every interior point has to be dropped, and the result has to be closed, which
// together pin down far more than a "did it return" check would.
//

typedef OpenTissue::math::BasicMathTypes<double, size_t> math_types;
typedef math_types::vector3_type                        vector3_type;
typedef OpenTissue::polymesh::PolyMesh<>                mesh_type;

namespace
{

  double const tolerance = 1e-6;

  /**
  * The eight corners of the unit cube [-1,1]^3.
  */
  std::vector<vector3_type> cube_corners()
  {
    std::vector<vector3_type> corners;
    for(int i = 0; i < 8; ++i)
      corners.push_back(vector3_type(
          (i & 1) ? 1.0 : -1.0
        , (i & 2) ? 1.0 : -1.0
        , (i & 4) ? 1.0 : -1.0
        ));
    return corners;
  }

} // anonymous namespace

BOOST_AUTO_TEST_SUITE(opentissue_mesh_convex_hull);

//
// The hull of a cube's corners plus points strictly inside it is the cube. The interior
// points must not survive, and no corner may be missed.
//
BOOST_AUTO_TEST_CASE(cube_with_interior_points)
{
  std::vector<vector3_type> points = cube_corners();

  // Points strictly inside, including the centre, none of which belong on the hull.
  points.push_back(vector3_type( 0.0,  0.0,  0.0));
  points.push_back(vector3_type( 0.5,  0.5,  0.5));
  points.push_back(vector3_type(-0.5,  0.25, 0.1));
  points.push_back(vector3_type( 0.9, -0.9,  0.0));
  points.push_back(vector3_type( 0.0,  0.99, 0.0));

  mesh_type mesh;
  OpenTissue::mesh::convex_hull(points.begin(), points.end(), mesh);

  BOOST_CHECK_EQUAL(mesh.size_vertices(), static_cast<size_t>(8));
  BOOST_CHECK(mesh.size_faces() > 0);

  // Every corner must be accounted for exactly once. Note the hull is computed with Qhull's
  // "QJ" option, which joggles the input, so positions come back perturbed by a tiny amount
  // rather than exactly as they went in.
  std::vector<vector3_type> const corners = cube_corners();
  std::vector<int> hits(corners.size(), 0);

  for(mesh_type::vertex_iterator v = mesh.vertex_begin(); v != mesh.vertex_end(); ++v)
  {
    int matched = -1;
    for(size_t c = 0; c < corners.size(); ++c)
    {
      vector3_type const d = v->m_coord - corners[c];
      if(std::sqrt(d * d) < tolerance)
        matched = static_cast<int>(c);
    }
    BOOST_REQUIRE_MESSAGE(matched >= 0, "hull vertex is not at any cube corner");
    ++hits[matched];
  }

  for(size_t c = 0; c < corners.size(); ++c)
    BOOST_CHECK_EQUAL(hits[c], 1);
}

//
// A closed hull: every edge is shared by two faces, so Euler's formula must hold. This is
// what catches a hull that comes back inside out or with facets missing -- the kind of
// breakage a wrong orientation or a half-initialised Qhull state would produce.
//
BOOST_AUTO_TEST_CASE(cube_hull_is_a_closed_surface)
{
  std::vector<vector3_type> const points = cube_corners();

  mesh_type mesh;
  OpenTissue::mesh::convex_hull(points.begin(), points.end(), mesh);

  int const V = static_cast<int>(mesh.size_vertices());
  int const E = static_cast<int>(mesh.size_edges());
  int const F = static_cast<int>(mesh.size_faces());

  BOOST_CHECK_EQUAL(V, 8);
  BOOST_CHECK_EQUAL(V - E + F, 2);
}

//
// Points on a sphere: every one of them is extreme, so all must survive, and nothing may be
// invented. This exercises a larger, non-degenerate input than the cube.
//
BOOST_AUTO_TEST_CASE(points_on_a_sphere_all_survive)
{
  std::vector<vector3_type> points;
  size_t const rings = 8;
  size_t const per_ring = 12;
  double const pi = 3.14159265358979323846;

  for(size_t i = 1; i < rings; ++i)
  {
    double const theta = pi * static_cast<double>(i) / static_cast<double>(rings);
    for(size_t j = 0; j < per_ring; ++j)
    {
      double const phi = 2.0 * pi * static_cast<double>(j) / static_cast<double>(per_ring);
      points.push_back(vector3_type(
          std::sin(theta) * std::cos(phi)
        , std::sin(theta) * std::sin(phi)
        , std::cos(theta)
        ));
    }
  }
  points.push_back(vector3_type(0.0, 0.0,  1.0));
  points.push_back(vector3_type(0.0, 0.0, -1.0));

  mesh_type mesh;
  OpenTissue::mesh::convex_hull(points.begin(), points.end(), mesh);

  BOOST_CHECK_EQUAL(mesh.size_vertices(), points.size());

  // Every hull vertex must still be on the unit sphere.
  for(mesh_type::vertex_iterator v = mesh.vertex_begin(); v != mesh.vertex_end(); ++v)
  {
    double const r = std::sqrt(v->m_coord * v->m_coord);
    BOOST_CHECK_CLOSE(r, 1.0, 1e-4);
  }

  BOOST_CHECK_EQUAL(static_cast<int>(mesh.size_vertices())
                  - static_cast<int>(mesh.size_edges())
                  + static_cast<int>(mesh.size_faces()), 2);
}

//
// Calling it twice in a row must give the same answer. Qhull's non-reentrant API keeps its
// state in globals, so a missing teardown shows up here and nowhere else.
//
BOOST_AUTO_TEST_CASE(repeated_calls_are_independent)
{
  std::vector<vector3_type> const points = cube_corners();

  mesh_type first;
  mesh_type second;
  OpenTissue::mesh::convex_hull(points.begin(), points.end(), first);
  OpenTissue::mesh::convex_hull(points.begin(), points.end(), second);

  BOOST_CHECK_EQUAL(first.size_vertices(), second.size_vertices());
  BOOST_CHECK_EQUAL(first.size_faces(),    second.size_faces());
  BOOST_CHECK_EQUAL(first.size_edges(),    second.size_edges());
}

BOOST_AUTO_TEST_SUITE_END();

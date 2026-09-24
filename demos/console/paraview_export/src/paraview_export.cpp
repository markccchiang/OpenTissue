//
// OpenTissue Template Library Demo
// - A specific demonstration of the flexibility of OTTL.
// Copyright (C) 2009 Department of Computer Science, University of Copenhagen.
//
// OTTL and OTTL Demos are licensed under zlib.
//
// OpenTissue -> ParaView, end to end. See documentation/paraview.md.
//
// Builds a box mesh, converts it to a signed distance field, and writes into the current
// directory:
//
//   box.obj                 the surface mesh
//   box_phi.mhd / .raw      its signed distance field
//   spin_0000.mhd ...       the field of the box turning, one file per frame
//
// Open any of them with File > Open in ParaView, or run render.py -- copied next to this
// program by the build -- with ParaView's pvbatch to render them without a window:
//
//   ./paraview_export
//   pvbatch render.py
//
#include <OpenTissue/configuration.h>

#include <OpenTissue/core/math/math_basic_types.h>
#include <OpenTissue/core/math/math_matrix3x3.h>
#include <OpenTissue/core/containers/mesh/mesh.h>
#include <OpenTissue/core/containers/mesh/common/io/mesh_obj_write.h>
#include <OpenTissue/core/containers/grid/grid.h>
#include <OpenTissue/core/containers/grid/util/grid_mesh2phi.h>
#include <OpenTissue/core/containers/grid/io/grid_metaimage_write.h>

#include <cstdio>
#include <iomanip>
#include <sstream>

typedef OpenTissue::math::BasicMathTypes<double, size_t>   math_types;
typedef math_types::matrix3x3_type                         matrix3x3_type;
typedef OpenTissue::polymesh::PolyMesh<math_types>         mesh_type;
typedef OpenTissue::grid::Grid<float, math_types>          grid_type;

int main()
{
  // 1. A surface: a 2 x 1 x 1 box centred on the origin.
  mesh_type box;
  OpenTissue::mesh::make_box(2.0, 1.0, 1.0, box);
  OpenTissue::mesh::obj_write("box.obj", box);

  // 2. Its signed distance field: negative inside, positive outside, zero on the surface.
  //    0.25 is the margin left around the mesh, 64 the number of voxels along each axis.
  //    (The shorter overload, mesh2phi(mesh, phi, 64), treats 64 only as an upper limit and
  //    derives the resolution from the smallest face -- 16 for a plain box, too coarse.)
  grid_type phi;
  OpenTissue::grid::mesh2phi(box, phi, 0.25, 64);
  OpenTissue::grid::metaimage_write("box_phi", phi);
  std::printf("box_phi: %u x %u x %u voxels, spacing %.4f, origin (%.3f, %.3f, %.3f)\n",
    unsigned(phi.I()), unsigned(phi.J()), unsigned(phi.K()), phi.dx(),
    phi.min_coord()(0), phi.min_coord()(1), phi.min_coord()(2));

  // 3. An animation: the box turning about z, one file per frame, numbered so ParaView
  //    groups them into a time series.
  //
  //    Every frame must use the SAME grid -- same origin, spacing and size. ParaView reads a
  //    series' grid geometry from its first file only and applies it to all the others, so
  //    if the grid changed per frame (as it would with mesh2phi(), which fits a new grid
  //    around the mesh each time) every later frame would be drawn distorted.
  //    So: create one grid, big enough for the whole motion, and refill it each frame.
  grid_type field;
  field.create(math_types::vector3_type(-1.4, -1.4, -0.75),    // min corner
               math_types::vector3_type( 1.4,  1.4,  0.75),    // max corner
               57, 57, 31);                                    // 0.05 voxels
  double const band = 2.0;   // compute distances this far from the surface: all of the grid

  matrix3x3_type const step = OpenTissue::math::Rz(3.14159265358979 / 20.0);   // 9 degrees
  for(int frame = 0; frame < 10; ++frame)
  {
    field.clear();                                            // back to "unused"
    OpenTissue::mesh::compute_angle_weighted_vertex_normals(box);
    OpenTissue::t4_cpu_scan(box, band, field, OpenTissue::t4_cpu_signed());

    std::ostringstream name;
    name << "spin_" << std::setw(4) << std::setfill('0') << frame;
    OpenTissue::grid::metaimage_write(name.str(), field);

    OpenTissue::mesh::rotate(box, step);
  }
  std::printf("wrote box.obj, box_phi.mhd and spin_0000.mhd .. spin_0009.mhd\n");
  return 0;
}

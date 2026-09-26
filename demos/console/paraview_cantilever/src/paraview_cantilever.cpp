//
// OpenTissue Template Library Demo
// - A specific demonstration of the flexibility of OTTL.
// Copyright (C) 2009 Department of Computer Science, University of Copenhagen.
//
// OTTL and OTTL Demos are licensed under zlib.
//
// A structural simulation, rendered in ParaView. See documentation/paraview_example_cantilever.md.
//
// A cantilever beam of soft rubber -- 4 m long, 0.5 m square, clamped at one end -- is released
// under its own weight. OpenTissue's finite element solver (dynamics/fem) computes how the
// elastic solid deforms: the beam droops a long way, swings, and settles hanging down, bent in
// a curve. A deflection this large is well beyond textbook beam theory, which assumes small
// sags; the checks it prints instead are ones that hold at any deflection.
//
// Written into the current directory, one file per frame:
//
//   beam_0000.vtk ...    the tetrahedral mesh, with per point the displacement from rest and
//                        per element the von Mises stress
//
// Then, in the same directory:
//
//   pvbatch render.py    renders frames to beam_*.png without opening a window
//
#include <OpenTissue/configuration.h>

#include <OpenTissue/core/math/math_basic_types.h>
#include <OpenTissue/core/math/math_matrix3x3.h>
#include <OpenTissue/core/containers/t4mesh/util/t4mesh_block_generator.h>
#include <OpenTissue/dynamics/fem/fem.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

typedef OpenTissue::math::BasicMathTypes<double, size_t>   math_types;
typedef math_types::vector3_type                           vector3_type;
typedef math_types::matrix3x3_type                         matrix3x3_type;
typedef OpenTissue::fem::Mesh<math_types>                  mesh_type;

// The beam: 4 m along x, 0.5 m high (y, the direction gravity acts in) and 0.5 m deep (z).
double const length  = 4.0;
double const side    = 0.5;
unsigned int const blocks_along  = 24;   // 24 x 4 x 4 blocks of 5 tetrahedra each: 1920 elements
unsigned int const blocks_across = 4;

// The material: soft rubber. Uniform, isotropic and purely elastic -- see the guide for what
// else these three numbers would describe.
double const young   = 5.0e6;    // Young's modulus, Pa: stiffness
double const poisson = 0.49;     // Poisson's ratio: rubber is nearly incompressible (0.5 is the limit)
double const density = 1100.0;   // kg / m^3
double const gravity = 9.81;     // m / s^2

// Time. The step is small for a reason: the solver runs a fixed 20 conjugate gradient
// iterations per step (see fem_simulate.h), and with larger steps that is not always enough.
// Measured on this beam: at dt = 0.005 the deepest point of the first swing comes out as
// 2.77 m at 1.75 s, at both 0.0025 and 0.001 as 2.60 m at 1.36 s, where it has stopped changing.
double       const dt              = 0.0025;
unsigned int const steps           = 2400;  // 6 seconds
unsigned int const steps_per_frame = 24;    // 101 frames, 0.06 s apart

/**
* Von Mises stress of one tetrahedron.
*
* The solver is corotational: each element's rotation m_Re is taken out before measuring
* strain, so a beam that swings rigidly is not "strained" by the swing. The same is done here.
* F is the deformation gradient, the strain is the symmetric part of R^T F minus the identity,
* and the stress follows from Hooke's law for an isotropic material.
*/
template<typename tetrahedron_type>
double von_mises(tetrahedron_type const & T)
{
  vector3_type const X0 = T.i()->m_model_coord, x0 = T.i()->m_coord;
  matrix3x3_type Dm, Ds;
  vector3_type const E1 = T.j()->m_model_coord - X0, E2 = T.k()->m_model_coord - X0, E3 = T.m()->m_model_coord - X0;
  vector3_type const e1 = T.j()->m_coord - x0,       e2 = T.k()->m_coord - x0,       e3 = T.m()->m_coord - x0;
  for(int r = 0; r < 3; ++r)
  {
    Dm(r, 0) = E1(r); Dm(r, 1) = E2(r); Dm(r, 2) = E3(r);
    Ds(r, 0) = e1(r); Ds(r, 1) = e2(r); Ds(r, 2) = e3(r);
  }
  matrix3x3_type const F  = Ds * OpenTissue::math::inverse(Dm);
  matrix3x3_type const RF = OpenTissue::math::trans(T.m_Re) * F;

  double eps[3][3];
  for(int r = 0; r < 3; ++r)
    for(int c = 0; c < 3; ++c)
      eps[r][c] = 0.5 * (RF(r, c) + RF(c, r)) - (r == c ? 1.0 : 0.0);

  double const lambda = young * poisson / ((1.0 + poisson) * (1.0 - 2.0 * poisson));
  double const mu     = young / (2.0 * (1.0 + poisson));
  double const tr     = eps[0][0] + eps[1][1] + eps[2][2];

  double s[3][3];
  for(int r = 0; r < 3; ++r)
    for(int c = 0; c < 3; ++c)
      s[r][c] = 2.0 * mu * eps[r][c] + (r == c ? lambda * tr : 0.0);

  return std::sqrt(0.5 * ((s[0][0] - s[1][1]) * (s[0][0] - s[1][1])
                        + (s[1][1] - s[2][2]) * (s[1][1] - s[2][2])
                        + (s[2][2] - s[0][0]) * (s[2][2] - s[0][0]))
                 + 3.0 * (s[0][1] * s[0][1] + s[1][2] * s[1][2] + s[2][0] * s[2][0]));
}

/**
* Write the mesh as a legacy VTK unstructured grid, which ParaView reads natively and groups
* into a time series when the files are numbered.
*/
void write_vtk(std::string const & filename, mesh_type & mesh)
{
  std::ofstream out(filename.c_str());
  out << "# vtk DataFile Version 3.0\n"
      << "OpenTissue FEM cantilever\n"
      << "ASCII\n"
      << "DATASET UNSTRUCTURED_GRID\n";

  out << "POINTS " << mesh.size_nodes() << " double\n";
  for(mesh_type::node_iterator n = mesh.node_begin(); n != mesh.node_end(); ++n)
    out << n->m_coord(0) << ' ' << n->m_coord(1) << ' ' << n->m_coord(2) << '\n';

  size_t const cells = mesh.size_tetrahedra();
  out << "CELLS " << cells << ' ' << 5 * cells << '\n';
  for(mesh_type::tetrahedron_iterator T = mesh.tetrahedron_begin(); T != mesh.tetrahedron_end(); ++T)
    out << "4 " << T->i()->idx() << ' ' << T->j()->idx() << ' ' << T->k()->idx() << ' ' << T->m()->idx() << '\n';

  out << "CELL_TYPES " << cells << '\n';
  for(size_t c = 0; c < cells; ++c)
    out << "10\n";                                           // VTK_TETRA

  out << "POINT_DATA " << mesh.size_nodes() << '\n'
      << "VECTORS displacement double\n";
  for(mesh_type::node_iterator n = mesh.node_begin(); n != mesh.node_end(); ++n)
  {
    vector3_type const u = n->m_coord - n->m_model_coord;
    out << u(0) << ' ' << u(1) << ' ' << u(2) << '\n';
  }

  out << "CELL_DATA " << cells << '\n'
      << "SCALARS von_mises double 1\n"
      << "LOOKUP_TABLE default\n";
  for(mesh_type::tetrahedron_iterator T = mesh.tetrahedron_begin(); T != mesh.tetrahedron_end(); ++T)
    out << von_mises(*T) << '\n';
}

int main()
{
  mesh_type beam;
  OpenTissue::t4mesh::generate_blocks(blocks_along, blocks_across, blocks_across,
    length / blocks_along, side / blocks_across, side / blocks_across, beam);

  // Record the rest shape, then give the material its properties. No plasticity: a yield
  // strain this large is never reached, so the beam stays purely elastic.
  OpenTissue::fem::update_original_coord(beam.node_begin(), beam.node_end());
  OpenTissue::fem::init(beam, young, poisson, density, 1.0e30, 0.0, 0.0);

  // Clamp the end at x = 0, and let gravity pull on every other node.
  std::vector<mesh_type::node_iterator> tip;
  for(mesh_type::node_iterator n = beam.node_begin(); n != beam.node_end(); ++n)
  {
    n->m_fixed = n->m_model_coord(0) < 1e-9;
    n->m_f_external = vector3_type(0.0, -gravity * n->m_mass, 0.0);
    if(n->m_model_coord(0) > length - 1e-9)
      tip.push_back(n);
  }

  // The beam's centre line: the nodes along the middle of its cross-section, in order.
  std::vector<mesh_type::node_iterator> centre;
  for(mesh_type::node_iterator n = beam.node_begin(); n != beam.node_end(); ++n)
    if(std::fabs(n->m_model_coord(1) - side / 2) < 1e-9 && std::fabs(n->m_model_coord(2) - side / 2) < 1e-9)
      centre.push_back(n);
  std::sort(centre.begin(), centre.end(),
    [](mesh_type::node_iterator const & a, mesh_type::node_iterator const & b) { return a->m_model_coord(0) < b->m_model_coord(0); });

  // Textbook (Euler-Bernoulli) beam theory assumes the sag is small next to the length. Printed
  // so the comparison is visible -- and so it is visible that it does not apply to this beam.
  double const area    = side * side;
  double const inertia = side * side * side * side / 12.0;
  double const sag     = density * gravity * area * std::pow(length, 4) / (8.0 * young * inertia);
  std::printf("beam: %zu nodes, %zu tetrahedra, %.0f kg of soft rubber\n",
    beam.size_nodes(), beam.size_tetrahedra(), density * area * length);
  std::printf("small-deflection beam theory would predict a %.2f m sag -- %.0f%% of the length, far\n"
              "beyond the few percent it is valid for. The checks below hold at any deflection.\n\n",
              sag, 100.0 * sag / length);

  // Bending should not stretch the centre line, however far the beam bends: its length is a
  // check that the large rotations are being handled, not mistaken for strain.
  auto centre_length = [&]()
  {
    double l = 0.0;
    for(size_t k = 1; k < centre.size(); ++k)
    {
      vector3_type const d = centre[k]->m_coord - centre[k - 1]->m_coord;
      l += std::sqrt(d * d);
    }
    return l;
  };
  auto tip_position = [&](double & down, double & out)
  {
    down = 0.0; out = 0.0;
    for(size_t k = 0; k < tip.size(); ++k)
    {
      down += tip[k]->m_model_coord(1) - tip[k]->m_coord(1);
      out  += tip[k]->m_coord(0);
    }
    down /= tip.size(); out /= tip.size();
  };

  std::printf("  time   tip down   tip out   centre line   largest stress\n");
  double deepest = 0.0, deepest_time = 0.0, down = 0.0, out = 0.0, longest = 0.0, shortest = 1e30;
  for(unsigned int step = 0; step <= steps; ++step)
  {
    if(step % steps_per_frame == 0)
    {
      std::ostringstream name;
      name << "beam_" << std::setw(4) << std::setfill('0') << step / steps_per_frame << ".vtk";
      write_vtk(name.str(), beam);
    }

    tip_position(down, out);
    if(down > deepest) { deepest = down; deepest_time = step * dt; }
    double const line = centre_length();
    longest = std::max(longest, line); shortest = std::min(shortest, line);

    if(step % 240 == 0)
    {
      double largest = 0.0;
      for(mesh_type::tetrahedron_iterator T = beam.tetrahedron_begin(); T != beam.tetrahedron_end(); ++T)
        largest = std::max(largest, von_mises(*T));
      std::printf("  %4.2f   %6.3f m   %6.3f m   %7.4f m   %9.0f Pa\n", step * dt, down, out, line, largest);
    }

    if(step < steps)
      OpenTissue::fem::simulate(beam, dt, true);
  }

  std::printf("\ndeepest point %.3f m below the clamp at %.2f s; settled %.3f m down and %.3f m out,\n"
              "%.3f m from the clamp in a straight line (the beam is %.1f m long, and bent)\n",
    deepest, deepest_time, down, out, std::sqrt(down * down + out * out), length);
  std::printf("centre line stayed between %.4f m and %.4f m long: stretched by at most %.2f%%\n",
    shortest, longest, 100.0 * (longest - length) / length);
  std::printf("wrote beam_0000.vtk .. beam_%04u.vtk\n", steps / steps_per_frame);
  return 0;
}

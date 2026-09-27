//
// OpenTissue Template Library Demo
// - A specific demonstration of the flexibility of OTTL.
// Copyright (C) 2009 Department of Computer Science, University of Copenhagen.
//
// OTTL and OTTL Demos are licensed under zlib.
//
// A multi-particle simulation, rendered in ParaView. See documentation/paraview_example_dam_break.md.
//
// A dam break: a column of water stands at one end of a tank, held by a dam, and at t = 0 the
// dam is gone. The water is about 1,200 particles moved by OpenTissue's smoothed particle
// hydrodynamics (SPH) solver: each particle carries a little mass of water, and the density,
// pressure and viscosity at a particle are sums over its neighbours within a smoothing radius.
// The column collapses, a wave runs along the floor, climbs the far wall and falls back, and
// the water sloshes to rest.
//
// Written into the current directory:
//
//   tank.vtk                   the tank, as an outline (written once; it does not move)
//   water_0000.vtk ...         the particles, one file per frame, with their velocity,
//                              speed, density and pressure
//
// Then, in the same directory:
//
//   pvbatch render.py          renders frames to water_*.png without opening a window
//
#include <OpenTissue/configuration.h>

// Find each particle's neighbours with a spatial hash grid. Without this the solver sums every
// particle against every other, which gives the same answer about eight times more slowly.
#define SPHSH

#include <OpenTissue/dynamics/sph/sph.h>
#include <OpenTissue/core/math/math_basic_types.h>
#include <OpenTissue/core/geometry/geometry_obb.h>
#include <OpenTissue/collision/spatial_hashing/spatial_hashing.h>
#include <OpenTissue/utility/utility_runtime_type.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

using namespace OpenTissue::math;

// The SPH solver takes the smoothing radius as a pointer template argument, so it has to be a
// global. It is set from the material below, before the solver is created.
typedef OpenTissue::utility::RuntimeType<double>  runtime_real_type;
runtime_real_type Radius;

typedef BasicMathTypes<double, int>                   math_types;
typedef math_types::vector3_type                      vector3_type;
typedef math_types::real_type                         real_type;
typedef OpenTissue::geometry::OBB<math_types>         box_type;

typedef OpenTissue::sph::Particle<real_type, Vector3, &Radius>                               particle_type;
typedef OpenTissue::sph::ImplicitBoxPrimitive<real_type, vector3_type, box_type>             implicit_box_type;
typedef OpenTissue::sph::ImplicitPrimitivesCollisionDetectionPolicy<real_type, vector3_type, particle_type>  collision_type;

typedef OpenTissue::sph::Types<
    real_type
  , Vector3
  , particle_type
  , collision_type
  , OpenTissue::spatial_hashing::PrimeNumberHashFunction
  , OpenTissue::spatial_hashing::Grid
  , OpenTissue::spatial_hashing::PointDataQuery
  > sph_types;

// The kernels check that a neighbour lies within the smoothing radius. The spatial hash only
// reports neighbours that do, but the check is cheap and keeps the sums right either way:
// beyond the radius the density kernel goes negative.
typedef OpenTissue::sph::WPoly6<sph_types, &Radius, true>      kernel_default;
typedef OpenTissue::sph::WSpiky<sph_types, &Radius, true>      kernel_pressure;
typedef OpenTissue::sph::WViscosity<sph_types, &Radius, true>  kernel_viscosity;

/**
* The pressure law, p = k (density - rest density), with negative pressures clamped to zero.
*
* OpenTissue's own Pressure gives negative pressure wherever the density is below rest, as it
* is at the free surface and next to the walls, where a particle has fewer neighbours to sum.
* Negative pressure pulls particles together: the water then packs itself against the floor
* in fewer, denser layers than its volume allows. Clamping it is a common remedy. The
* solver takes the pressure law as a policy, so the demo can supply its own.
*/
class ClampedPressure : public OpenTissue::sph::Solver< sph_types, sph_types::real_type >
{
public:

  typedef sph_types::real_type                                        real_type;
  typedef sph_types::particle                                         particle;
  typedef sph_types::particle_cptr_container::const_iterator          particle_iterator;

  ClampedPressure(real_type const & stiffness, real_type const & rest_density)
    : m_k(stiffness)
    , m_rest_density(rest_density)
  {}

  real_type apply(particle const & par, particle_iterator, particle_iterator) const
  {
    return std::max( real_type(0), m_k * ( par.density() - m_rest_density ) );
  }

protected:

  real_type m_k;
  real_type m_rest_density;
};

typedef OpenTissue::sph::System<
    sph_types
  , OpenTissue::sph::Density<sph_types, kernel_default>
  , ClampedPressure
  , OpenTissue::sph::SurfaceNormal<sph_types, kernel_default>
  , OpenTissue::sph::Gravity<sph_types>
  , OpenTissue::sph::Buoyancy<sph_types>
  , OpenTissue::sph::PressureForce<sph_types, kernel_pressure>
  , OpenTissue::sph::ViscosityForce<sph_types, kernel_viscosity>
  , OpenTissue::sph::SurfaceForce<sph_types, kernel_default>
  , OpenTissue::sph::LeapFrog<sph_types>
  , OpenTissue::sph::ColorField<sph_types, kernel_default>
  > sph_system_type;

/**
* OpenTissue's water, made stiffer. SPH water is slightly compressible, by how much set by the
* gas stiffness, and a stiffer fluid needs a shorter time step to stay stable. The library's
* water pairs a stiffness of 3 with a 0.01 s step, which lets the column squeeze to twice its
* density as it collapses and leaves it 20% compressed at rest. A stiffness of 40 with a
* 0.0025 s step keeps it within a few percent of the density of water throughout, for four
* times the steps.
*/
class StiffWater : public OpenTissue::sph::Water<sph_types>
{
public:

  StiffWater()
  {
    m_timestep      = 0.0025;
    m_gas_stiffness = 40.0;
  }
};

typedef StiffWater water_type;

// The tank, in metres: 1 long, 0.2 wide, 0.6 high, with its floor at z = 0.
double const tank_length = 1.0;
double const tank_width  = 0.2;
double const tank_height = 0.6;

// The column of water, against the tank's end wall at x = 0.
double const column_length = 0.3;
double const column_height = 0.45;

// Time. The water material sets the solver's step, 0.0025 s.
unsigned int const steps           = 1200;  // 3 seconds
unsigned int const steps_per_frame = 8;     // 151 frames, 0.02 s apart
unsigned int const steps_per_line  = 100;   // a line of checks every 0.25 s
double       const gravity         = 9.82;

/**
* Writes the particles as legacy VTK polydata: one vertex per particle, with the particle's
* velocity, speed, density and pressure as point data.
*/
void write_particles(std::string const & name, sph_system_type const & sph)
{
  sph_types::particle_container const & particles = sph.particles();
  size_t const n = particles.size();

  std::ofstream out(name.c_str());
  out << "# vtk DataFile Version 3.0\n"
      << "OpenTissue SPH particles\n"
      << "ASCII\n"
      << "DATASET POLYDATA\n"
      << "POINTS " << n << " double\n";
  for(sph_types::particle_container::const_iterator p = particles.begin(); p != particles.end(); ++p)
    out << p->position()(0) << " " << p->position()(1) << " " << p->position()(2) << "\n";

  out << "VERTICES " << n << " " << 2 * n << "\n";
  for(size_t i = 0; i < n; ++i)
    out << "1 " << i << "\n";

  out << "POINT_DATA " << n << "\n"
      << "VECTORS velocity double\n";
  for(sph_types::particle_container::const_iterator p = particles.begin(); p != particles.end(); ++p)
    out << p->velocity()(0) << " " << p->velocity()(1) << " " << p->velocity()(2) << "\n";

  out << "SCALARS speed double 1\nLOOKUP_TABLE default\n";
  for(sph_types::particle_container::const_iterator p = particles.begin(); p != particles.end(); ++p)
    out << length(p->velocity()) << "\n";

  out << "SCALARS density double 1\nLOOKUP_TABLE default\n";
  for(sph_types::particle_container::const_iterator p = particles.begin(); p != particles.end(); ++p)
    out << p->density() << "\n";

  out << "SCALARS pressure double 1\nLOOKUP_TABLE default\n";
  for(sph_types::particle_container::const_iterator p = particles.begin(); p != particles.end(); ++p)
    out << p->pressure() << "\n";
}

/**
* Writes the tank as the twelve edges of a box, as legacy VTK polydata lines.
*/
void write_tank(std::string const & name)
{
  std::ofstream out(name.c_str());
  out << "# vtk DataFile Version 3.0\n"
      << "OpenTissue SPH tank\n"
      << "ASCII\n"
      << "DATASET POLYDATA\n"
      << "POINTS 8 double\n";
  for(int k = 0; k < 2; ++k)
    for(int j = 0; j < 2; ++j)
      for(int i = 0; i < 2; ++i)
        out << i * tank_length << " " << j * tank_width << " " << k * tank_height << "\n";
  // Corner index = i + 2 j + 4 k; an edge joins corners that differ in one coordinate.
  out << "LINES 12 36\n";
  int const edges[12][2] = { {0,1},{2,3},{4,5},{6,7}, {0,2},{1,3},{4,6},{5,7}, {0,4},{1,5},{2,6},{3,7} };
  for(int e = 0; e < 12; ++e)
    out << "2 " << edges[e][0] << " " << edges[e][1] << "\n";
}

int main()
{
  // Particles start on a lattice at water's rest spacing, the cube root of the volume one
  // particle's mass of water takes up, so the column starts close to rest density.
  water_type material;
  double const spacing = std::cbrt( material.particle_mass() / material.density() );

  size_t const nx = static_cast<size_t>( column_length / spacing );
  size_t const ny = static_cast<size_t>( tank_width    / spacing );
  size_t const nz = static_cast<size_t>( column_height / spacing );
  size_t const n  = nx * ny * nz;

  std::vector<vector3_type> positions, velocities;
  for(size_t k = 0; k < nz; ++k)
    for(size_t j = 0; j < ny; ++j)
      for(size_t i = 0; i < nx; ++i)
      {
        positions.push_back( vector3_type( (i + 0.5) * spacing, (j + 0.5) * spacing, (k + 0.5) * spacing ) );
        velocities.push_back( vector3_type( 0.0, 0.0, 0.0 ) );
      }

  // The material works out the smoothing radius from how many neighbours a particle should
  // have within it, and the solver reads it through the global Radius.
  material.particles() = n;
  material.particle_mass( material.particle_mass() );
  real_type const neighbours = material.kernel_particles();
  material.threshold() = material.density() / neighbours;
  Radius = material.radius( neighbours );

  sph_system_type sph;
  if( !sph.create( material, vector3_type( 0.0, 0.0, -gravity ) ) )
  {
    std::fprintf(stderr, "could not create the SPH system\n");
    return 1;
  }
  if( !sph.initHashing( 2u * n, Radius ) )
  {
    std::fprintf(stderr, "could not create the spatial hash grid\n");
    return 1;
  }

  // The tank holds the particles in. The solver keeps particle centres inside it, and a
  // particle stands for water half a spacing around its centre, so the box the solver sees is
  // half a spacing smaller on every side than the tank drawn in ParaView.
  double const margin = 0.5 * spacing;
  box_type tank( vector3_type( 0.5 * tank_length, 0.5 * tank_width, 0.5 * tank_height )
               , diag( 1.0 )
               , vector3_type( 0.5 * tank_length - margin, 0.5 * tank_width - margin, 0.5 * tank_height - margin ) );
  implicit_box_type implicit_tank( tank );
  sph.collisionSystem().addContainer( implicit_tank );

  if( !sph.init( positions.begin(), positions.end(), velocities.begin(), velocities.end() ) )
  {
    std::fprintf(stderr, "could not place the particles\n");
    return 1;
  }

  write_tank( "tank.vtk" );

  // Checks that this is behaving like water, printed as it runs:
  //  - no particle leaves the tank;
  //  - the density stays close to that of water, 998 kg/m^3: water barely compresses;
  //  - the water comes to rest: its mean speed falls towards zero once the sloshing dies out.
  std::printf("%zu particles, spacing %.4f m, smoothing radius %.4f m, time step %.4f s\n"
    , n, spacing, static_cast<double>(Radius), material.timestep());
  std::printf("  time   front   top     outside   density: mean   lowest   highest   mean speed\n");

  for(unsigned int step = 0; step <= steps; ++step)
  {
    if(step % steps_per_frame == 0)
    {
      std::ostringstream name;
      name << "water_" << std::setw(4) << std::setfill('0') << step / steps_per_frame << ".vtk";
      write_particles( name.str(), sph );
    }

    if(step % steps_per_line == 0)
    {
      double front = 0.0, top = 0.0, density = 0.0, speed = 0.0;
      double lowest = 1e300, highest = 0.0;
      size_t outside = 0;
      for(sph_types::particle_container::const_iterator p = sph.particles().begin(); p != sph.particles().end(); ++p)
      {
        vector3_type const & x = p->position();
        // Only particles near the floor mark the front, not spray thrown ahead of it.
        if( x(2) < 2.0 * spacing )
          front = std::max( front, x(0) );
        top      = std::max( top, x(2) );
        density += p->density();
        lowest   = std::min( lowest,  static_cast<double>( p->density() ) );
        highest  = std::max( highest, static_cast<double>( p->density() ) );
        speed   += length( p->velocity() );
        double const slack = 1e-9;
        if( x(0) < -slack || x(0) > tank_length + slack || x(1) < -slack || x(1) > tank_width + slack || x(2) < -slack || x(2) > tank_height + slack )
          ++outside;
      }
      std::printf("  %4.2f   %5.3f   %5.3f   %4zu      %7.1f       %6.1f   %7.1f    %6.3f\n"
        , step * material.timestep(), front, top, outside, density / n, lowest, highest, speed / n );
    }

    if(step < steps)
      sph.simulate();
  }

  std::printf("wrote tank.vtk and water_0000.vtk .. water_%04u.vtk\n", steps / steps_per_frame);
  return 0;
}

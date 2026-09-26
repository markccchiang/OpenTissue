//
// OpenTissue Template Library Demo
// - A specific demonstration of the flexibility of OTTL.
// Copyright (C) 2009 Department of Computer Science, University of Copenhagen.
//
// OTTL and OTTL Demos are licensed under zlib.
//
// A physical simulation, rendered in ParaView. See documentation/paraview_example_shallow_water.md.
//
// A drop of water falls into a pool 10 units across and 1 unit deep, with a hill on the
// bottom. OpenTissue's shallow water solver moves the water; the rings spread out, bounce off
// the walls, and slow down where they cross the shallow water over the hill -- in shallow
// water, waves travel at sqrt(g * depth).
//
// Written into the current directory:
//
//   sea_bed.mhd / .raw         the bottom of the pool (written once; it does not move)
//   water_0000.mhd ...         the water surface, one file per frame
//
// Then, in the same directory:
//
//   pvbatch render.py          renders frames to water_*.png without opening a window
//
#include <OpenTissue/configuration.h>

#include <OpenTissue/core/math/math_basic_types.h>
#include <OpenTissue/core/containers/grid/grid.h>
#include <OpenTissue/core/containers/grid/io/grid_metaimage_write.h>
#include <OpenTissue/dynamics/swe/swe_shallow_water_equation.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <string>

typedef OpenTissue::math::BasicMathTypes<double, size_t>   math_types;
typedef math_types::vector3_type                           vector3_type;
typedef OpenTissue::grid::Grid<float, math_types>          grid_type;
typedef OpenTissue::swe::ShallowWaterEquations<double>     water_type;

// The pool.
unsigned int const nodes    = 80;       // grid nodes along each side
double       const width    = 10.0;     // side length
double       const level    = 1.0;      // water level at rest
double       const spacing  = width / nodes;

// The drop, and the hill on the bottom.
double const drop_x = 2.5,  drop_y = 5.0, drop_height = 0.25, drop_radius = 0.6;
double const hill_x = 6.5,  hill_y = 5.0, hill_height = 0.75, hill_radius = 1.2;

// Time.
double       const dt              = 0.01;
unsigned int const steps           = 600;   // 6 seconds
unsigned int const steps_per_frame = 6;     // 101 frames, 0.06 s apart

double bump(double x, double y, double cx, double cy, double height, double radius)
{
  double const r2 = (x - cx) * (x - cx) + (y - cy) * (y - cy);
  return height * std::exp(-r2 / (radius * radius));
}

/**
* Writes height fields -- a height for each (x, y) node -- so that ParaView can show them as
* surfaces.
*
* An OpenTissue grid is three-dimensional, so each height field is stored in a grid only two
* layers deep, at z = z_low and z = z_high, holding z minus the height. That is zero exactly at
* the height, and because it is linear in z, ParaView's Contour filter at value 0 recovers the
* surface exactly: the same "zero level of a field" idea as a signed distance field.
*
* The grid is created once and reused, so every file it writes has the same geometry, which a
* ParaView time series needs: ParaView takes a series' grid geometry from its first file.
* z_low and z_high must bracket every height that will be written.
*/
class SurfaceWriter
{
public:

  SurfaceWriter(double z_low, double z_high)
    : m_z_low(z_low)
    , m_z_high(z_high)
  {
    m_field.create(vector3_type(0.0, 0.0, z_low),
                   vector3_type((nodes - 1) * spacing, (nodes - 1) * spacing, z_high),
                   nodes, nodes, 2);
  }

  template<typename height_function>
  void write(std::string const & name, height_function height)
  {
    for(unsigned int j = 0; j < nodes; ++j)
      for(unsigned int i = 0; i < nodes; ++i)
      {
        double const h = height(i, j);
        m_field(i, j, 0) = static_cast<float>(m_z_low  - h);
        m_field(i, j, 1) = static_cast<float>(m_z_high - h);
      }
    OpenTissue::grid::metaimage_write(name, m_field);
  }

protected:

  double    m_z_low;
  double    m_z_high;
  grid_type m_field;
};

int main()
{
  water_type water;
  water.init(nodes, nodes, width, width);

  for(unsigned int i = 0; i < nodes; ++i)
    for(unsigned int j = 0; j < nodes; ++j)
    {
      double const x = i * spacing, y = j * spacing;
      water.setSeaHeight(i, j, level + bump(x, y, drop_x, drop_y, drop_height, drop_radius));
      water.setSeaBottom(i, j, bump(x, y, hill_x, hill_y, hill_height, hill_radius));
    }

  // One writer for both surfaces, so the sea bed and the water line up in ParaView.
  SurfaceWriter surface(-0.1, 1.6);

  surface.write("sea_bed", [&](unsigned int i, unsigned int j) { return water.getSeaBottom(i, j); });

  // Checks that this is behaving like water, printed as it runs:
  //  - the volume of water must not change: nothing enters or leaves the pool;
  //  - the ring's crest should travel at about sqrt(g * depth), 3.1 units a second at depth 1.
  auto volume = [&]()
  {
    double v = 0.0;
    for(unsigned int i = 0; i < nodes; ++i)
      for(unsigned int j = 0; j < nodes; ++j)
        v += (water.getSeaHeight(i, j) - water.getSeaBottom(i, j)) * spacing * spacing;
    return v;
  };
  // Where the crest is along the line from the drop towards y = 10, the far wall, over
  // water of depth 1 that the hill does not reach.
  auto crest = [&]()
  {
    unsigned int const i = static_cast<unsigned int>(drop_x / spacing + 0.5);
    unsigned int const j0 = static_cast<unsigned int>(drop_y / spacing + 0.5);
    unsigned int best = j0;
    for(unsigned int j = j0; j < nodes; ++j)
      if(water.getSeaHeight(i, j) > water.getSeaHeight(i, best))
        best = j;
    return (best - j0) * spacing;
  };

  double const start_volume = volume();
  std::printf("  time   volume    change   highest   crest distance\n");

  for(unsigned int step = 0; step <= steps; ++step)
  {
    if(step % steps_per_frame == 0)
    {
      std::ostringstream name;
      name << "water_" << std::setw(4) << std::setfill('0') << step / steps_per_frame;
      surface.write(name.str(), [&](unsigned int i, unsigned int j) { return water.getSeaHeight(i, j); });
    }

    if(step % 50 == 0)
    {
      double highest = 0.0;
      for(unsigned int i = 0; i < nodes; ++i)
        for(unsigned int j = 0; j < nodes; ++j)
          highest = std::max(highest, water.getSeaHeight(i, j));
      double const v = volume();
      std::printf("  %4.2f  %8.4f  %+7.4f%%  %7.4f   %6.3f\n",
        step * dt, v, 100.0 * (v - start_volume) / start_volume, highest, crest());
    }

    if(step < steps)
      water.run(dt);
  }

  std::printf("wrote sea_bed.mhd and water_0000.mhd .. water_%04u.mhd\n", steps / steps_per_frame);
  return 0;
}

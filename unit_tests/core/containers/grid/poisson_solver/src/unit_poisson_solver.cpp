//
// OpenTissue, A toolbox for physical based simulation and animation.
// Copyright (C) 2007 Department of Computer Science, University of Copenhagen
//
#include <OpenTissue/configuration.h>

#include <OpenTissue/core/math/math_basic_types.h>
#include <OpenTissue/core/containers/grid/grid.h>
#include <OpenTissue/core/containers/grid/util/grid_poisson_solver.h>
#include <OpenTissue/core/containers/grid/util/grid_laplacian_blur.h>

#define BOOST_AUTO_TEST_MAIN
#include <OpenTissue/utility/utility_push_boost_filter.h>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>
#include <boost/test/tools/floating_point_comparison.hpp>
#include <boost/test/test_tools.hpp>
#include <OpenTissue/utility/utility_pop_boost_filter.h>

#include <algorithm>
#include <cmath>
#include <cstddef>

typedef OpenTissue::math::BasicMathTypes<double, size_t>  math_types;
typedef math_types::vector3_type                          vector3_type;
typedef OpenTissue::grid::Grid<double, math_types>        grid_type;

namespace
{
  /**
  * A grid of I x J x K nodes spaced dx, dy and dz apart.
  */
  grid_type make_grid(size_t I, size_t J, size_t K, double dx, double dy, double dz)
  {
    grid_type grid;
    grid.create(vector3_type(0.0, 0.0, 0.0), vector3_type((I - 1) * dx, (J - 1) * dy, (K - 1) * dz), I, J, K);
    return grid;
  }

  /**
  * A smooth field with no symmetry that could hide an indexing mistake.
  */
  void fill_smooth(grid_type & phi)
  {
    for(size_t k = 0; k < phi.K(); ++k)
      for(size_t j = 0; j < phi.J(); ++j)
        for(size_t i = 0; i < phi.I(); ++i)
          phi(i, j, k) = std::sin(0.7 * i + 0.1) * std::cos(0.4 * j) + 0.3 * std::cos(0.9 * k + 0.2 * i);
  }

  /**
  * The discrete Laplacian of phi with the solver's boundary condition: an index outside the
  * grid is clamped onto the boundary, which makes the normal derivative zero there.
  */
  grid_type laplacian(grid_type const & phi)
  {
    grid_type W = phi;
    size_t const I = phi.I(), J = phi.J(), K = phi.K();
    double const dx2 = phi.dx() * phi.dx(), dy2 = phi.dy() * phi.dy(), dz2 = phi.dz() * phi.dz();
    for(size_t k = 0; k < K; ++k)
      for(size_t j = 0; j < J; ++j)
        for(size_t i = 0; i < I; ++i)
        {
          double const c = phi(i, j, k);
          double const xx = phi(i ? i - 1 : 0, j, k) + phi(std::min(i + 1, I - 1), j, k) - 2.0 * c;
          double const yy = phi(i, j ? j - 1 : 0, k) + phi(i, std::min(j + 1, J - 1), k) - 2.0 * c;
          double const zz = phi(i, j, k ? k - 1 : 0) + phi(i, j, std::min(k + 1, K - 1)) - 2.0 * c;
          W(i, j, k) = xx / dx2 + yy / dy2 + zz / dz2;
        }
    return W;
  }

  /**
  * If W is the discrete Laplacian of phi, then phi solves the discrete Poisson equation
  * exactly, and a Gauss-Seidel sweep, which solves each node's equation for that node, must
  * leave every value as it is.
  */
  double largest_change_after_one_sweep(grid_type const & phi)
  {
    grid_type const W = laplacian(phi);
    grid_type swept = phi;
    OpenTissue::grid::poisson_solver(swept, W, 1u);

    double largest = 0.0;
    for(size_t k = 0; k < phi.K(); ++k)
      for(size_t j = 0; j < phi.J(); ++j)
        for(size_t i = 0; i < phi.I(); ++i)
          largest = std::max(largest, std::fabs(swept(i, j, k) - phi(i, j, k)));
    return largest;
  }
}

BOOST_AUTO_TEST_SUITE(opentissue_grid_poisson_solver);

// The equal-spacing branch once divided by 8 instead of 6, which every test below catches.

BOOST_AUTO_TEST_CASE(solution_is_a_fixed_point_on_a_uniform_grid)
{
  grid_type phi = make_grid(7, 6, 5, 0.25, 0.25, 0.25);
  fill_smooth(phi);
  BOOST_CHECK_SMALL(largest_change_after_one_sweep(phi), 1e-9);
}

BOOST_AUTO_TEST_CASE(solution_is_a_fixed_point_on_a_non_uniform_grid)
{
  grid_type phi = make_grid(7, 6, 5, 0.25, 0.5, 0.2);
  fill_smooth(phi);
  BOOST_CHECK_SMALL(largest_change_after_one_sweep(phi), 1e-9);
}

BOOST_AUTO_TEST_CASE(constant_field_solves_the_laplace_equation)
{
  // With a zero right hand side, a constant field is a solution under Neumann boundary
  // conditions, so no number of sweeps may change it.
  grid_type phi = make_grid(6, 6, 6, 0.1, 0.1, 0.1);
  std::fill(phi.begin(), phi.end(), 2.5);
  grid_type W = phi;
  std::fill(W.begin(), W.end(), 0.0);

  OpenTissue::grid::poisson_solver(phi, W, 20u);

  for(grid_type::iterator p = phi.begin(); p != phi.end(); ++p)
    BOOST_CHECK_CLOSE(*p, 2.5, 1e-9);
}

BOOST_AUTO_TEST_CASE(sweeps_converge_to_the_solution)
{
  // Starting from zero, repeated sweeps must approach a known solution. The pure Neumann
  // problem fixes the solution only up to a constant, so compare after removing the mean.
  //
  // The spacing is a power of two on purpose. The solver picks its equal-spacing branch by
  // comparing dx, dy and dz exactly, and a spacing such as 0.2 is not exact in binary: the
  // grid's computed spacings then differ in the last bit and the other branch runs.
  grid_type exact = make_grid(6, 5, 4, 0.25, 0.25, 0.25);
  fill_smooth(exact);
  grid_type const W = laplacian(exact);

  grid_type phi = exact;
  std::fill(phi.begin(), phi.end(), 0.0);
  OpenTissue::grid::poisson_solver(phi, W, 2000u);

  double mean_exact = 0.0, mean_phi = 0.0;
  size_t n = 0;
  for(grid_type::iterator e = exact.begin(), p = phi.begin(); e != exact.end(); ++e, ++p, ++n)
  {
    mean_exact += *e;
    mean_phi   += *p;
  }
  mean_exact /= n;
  mean_phi   /= n;

  double largest = 0.0;
  for(grid_type::iterator e = exact.begin(), p = phi.begin(); e != exact.end(); ++e, ++p)
    largest = std::max(largest, std::fabs((*p - mean_phi) - (*e - mean_exact)));
  BOOST_CHECK_SMALL(largest, 1e-6);
}

BOOST_AUTO_TEST_CASE(laplacian_blur_preserves_a_constant_image)
{
  // Blurring must not change an image with nothing to blur. With the old factor of 1/8 each
  // sweep scaled the image by 6/8, so it faded towards zero instead.
  grid_type image = make_grid(6, 6, 6, 1.0, 1.0, 1.0);
  std::fill(image.begin(), image.end(), 1.0);

  OpenTissue::grid::laplacian_blur(image, 1.0, 10u);

  for(grid_type::iterator p = image.begin(); p != image.end(); ++p)
    BOOST_CHECK_CLOSE(*p, 1.0, 1e-9);
}

BOOST_AUTO_TEST_SUITE_END();

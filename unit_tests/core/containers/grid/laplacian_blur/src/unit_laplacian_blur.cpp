//
// OpenTissue, A toolbox for physical based simulation and animation.
// Copyright (C) 2007 Department of Computer Science, University of Copenhagen
//
#include <OpenTissue/configuration.h>

#include <OpenTissue/core/math/math_basic_types.h>
#include <OpenTissue/core/containers/grid/grid.h>
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
#include <limits>

typedef OpenTissue::math::BasicMathTypes<double, size_t>  math_types;
typedef math_types::vector3_type                          vector3_type;
typedef OpenTissue::grid::Grid<double, math_types>        grid_type;

namespace
{
  grid_type make_grid(size_t I, size_t J, size_t K, double dx, double dy, double dz)
  {
    grid_type grid;
    grid.create(vector3_type(0.0, 0.0, 0.0), vector3_type((I - 1) * dx, (J - 1) * dy, (K - 1) * dz), I, J, K);
    return grid;
  }

  /**
  * An image with some structure, and no symmetry that could hide an indexing mistake.
  */
  grid_type make_image()
  {
    grid_type image = make_grid(7, 6, 5, 0.5, 0.25, 0.5);
    for(size_t k = 0; k < image.K(); ++k)
      for(size_t j = 0; j < image.J(); ++j)
        for(size_t i = 0; i < image.I(); ++i)
          image(i, j, k) = std::sin(0.7 * i + 0.1) * std::cos(0.4 * j) + 0.3 * std::cos(0.9 * k + 0.2 * i) + 1.0;
    return image;
  }

  /**
  * The largest absolute difference, or infinity if either grid holds a NaN -- which
  * std::max would otherwise quietly skip.
  */
  double largest_difference(grid_type const & a, grid_type const & b)
  {
    double largest = 0.0;
    for(size_t k = 0; k < a.K(); ++k)
      for(size_t j = 0; j < a.J(); ++j)
        for(size_t i = 0; i < a.I(); ++i)
        {
          double const d = std::fabs(a(i, j, k) - b(i, j, k));
          if(std::isnan(d))
            return std::numeric_limits<double>::infinity();
          largest = std::max(largest, d);
        }
    return largest;
  }

  double sum(grid_type const & a)
  {
    double s = 0.0;
    for(grid_type::const_iterator p = a.begin(); p != a.end(); ++p)
      s += *p;
    return s;
  }

  /**
  * phi - nu * laplacian(phi), with the Neumann boundary condition the blur uses: an index
  * outside the grid is clamped onto the boundary.
  */
  grid_type apply_operator(grid_type const & phi, double nu)
  {
    grid_type result = phi;
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
          result(i, j, k) = c - nu * (xx / dx2 + yy / dy2 + zz / dz2);
        }
    return result;
  }
}

BOOST_AUTO_TEST_SUITE(opentissue_grid_laplacian_blur);

BOOST_AUTO_TEST_CASE(solves_the_implicit_diffusion_equation)
{
  // Blurring phi_0 = phi - nu laplacian(phi) must give back phi, for any phi. Gauss-Seidel
  // converges more slowly the larger the diffusion, hence the generous iteration count.
  grid_type const phi = make_image();
  double const diffusions[] = { 0.01, 0.1, 1.0, 5.0 };
  for(size_t d = 0; d < sizeof(diffusions)/sizeof(diffusions[0]); ++d)
  {
    BOOST_TEST_CONTEXT("diffusion " << diffusions[d])
    {
      grid_type blurred = apply_operator(phi, diffusions[d]);
      OpenTissue::grid::laplacian_blur(blurred, diffusions[d], 20000u);
      BOOST_CHECK_SMALL(largest_difference(blurred, phi), 1e-9);
    }
  }
}

BOOST_AUTO_TEST_CASE(zero_diffusion_leaves_the_image_alone)
{
  grid_type const image = make_image();
  grid_type blurred = image;
  OpenTissue::grid::laplacian_blur(blurred, 0.0, 10u);
  BOOST_CHECK_EQUAL(largest_difference(blurred, image), 0.0);
}

BOOST_AUTO_TEST_CASE(blur_changes_continuously_with_diffusion)
{
  // The blur used to replace the right-hand side by zero for diffusion 1 exactly, so 1.0
  // and 1.0001 gave entirely different images.
  grid_type a = make_image();
  grid_type b = make_image();
  OpenTissue::grid::laplacian_blur(a, 1.0, 50u);
  OpenTissue::grid::laplacian_blur(b, 1.0001, 50u);
  BOOST_CHECK_SMALL(largest_difference(a, b), 1e-3);
}

BOOST_AUTO_TEST_CASE(constant_image_stays_constant)
{
  double const diffusions[] = { 0.1, 1.0, 3.0 };
  for(size_t d = 0; d < sizeof(diffusions)/sizeof(diffusions[0]); ++d)
  {
    grid_type image = make_grid(6, 6, 6, 1.0, 1.0, 1.0);
    std::fill(image.begin(), image.end(), 1.0);
    OpenTissue::grid::laplacian_blur(image, diffusions[d], 10u);
    for(grid_type::iterator p = image.begin(); p != image.end(); ++p)
      BOOST_CHECK_CLOSE(*p, 1.0, 1e-9);
  }
}

BOOST_AUTO_TEST_CASE(blur_spreads_a_spike_and_keeps_its_mass)
{
  // No flux through the boundary, so the sum of the values is preserved, while the peak
  // spreads out.
  grid_type image = make_grid(9, 9, 9, 1.0, 1.0, 1.0);
  std::fill(image.begin(), image.end(), 0.0);
  image(4, 4, 4) = 1.0;

  OpenTissue::grid::laplacian_blur(image, 1.0, 500u);

  BOOST_CHECK_CLOSE(sum(image), 1.0, 1e-6);
  BOOST_CHECK(image(4, 4, 4) < 0.5);
  BOOST_CHECK(image(5, 4, 4) > 0.0);
  BOOST_CHECK_CLOSE(image(5, 4, 4), image(3, 4, 4), 1e-6);
}

BOOST_AUTO_TEST_CASE(negative_diffusion_is_rejected)
{
  grid_type image = make_image();
  BOOST_CHECK_THROW(OpenTissue::grid::laplacian_blur(image, -1.0, 10u), std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END();

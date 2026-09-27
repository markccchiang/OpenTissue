#ifndef OPENTISSUE_CORE_CONTAINERS_GRID_UTIL_GRID_LAPLACIAN_BLUR_H
#define OPENTISSUE_CORE_CONTAINERS_GRID_UTIL_GRID_LAPLACIAN_BLUR_H
//
// OpenTissue Template Library
// - A generic toolbox for physics-based modeling and simulation.
// Copyright (C) 2008 Department of Computer Science, University of Copenhagen.
//
// OTTL is licensed under zlib: http://opensource.org/licenses/zlib-license.php
//
#include <OpenTissue/configuration.h>

#include <algorithm>
#include <cstddef>
#include <stdexcept>

namespace OpenTissue
{
  namespace grid
  {
    /**
    * Blur by Diffusion.
    * Takes one implicit (backward Euler) step of the heat equation, of length diffusion,
    * by solving
    *
    * \f[ \phi - \nu \nabla^2 \phi = \phi_0 \f]
    *
    * for \f$\phi\f$, where \f$\phi_0\f$ is the input image and \f$\nu\f$ the diffusion
    * coefficient. \f$\nu = 0\f$ leaves the image as it is, and the blur grows smoothly with
    * \f$\nu\f$. The boundary conditions are pure Neumann (no flux through the boundary), so a
    * constant image stays constant and the sum of the values is preserved.
    *
    * The equation is solved by Gauss-Seidel iteration, starting from the image itself. Its
    * matrix is diagonally dominant, so the iteration converges for every diffusion
    * coefficient; the larger the coefficient, the more iterations it takes.
    *
    * This used to solve \f$\nu \nabla^2 \phi = \phi_0\f$ with the Poisson solver instead.
    * With Neumann boundary conditions that equation has no solution unless the image
    * averages to zero, and for \f$\nu = 1\f$ the code replaced the right-hand side by
    * zero, so the result jumped between \f$\nu = 1\f$ and any other value.
    *
    * @param image            The image to blur. Upon return it holds the blurred image.
    * @param diffusion        The diffusion coefficient, the length of the diffusion step. Must
    *                         be non-negative.
    * @param max_iterations   The number of Gauss-Seidel iterations. Default is 10 iterations.
    */
    template<typename grid_type>
    inline void laplacian_blur(
      grid_type & image
      , double diffusion=1.0
      , size_t max_iterations = 10
      )
    {
      using std::min;

      typedef typename grid_type::value_type            value_type;
      typedef typename grid_type::math_types            math_types;
      typedef typename math_types::real_type            real_type;
      typedef typename math_types::value_traits         value_traits;

      if(diffusion < 0.0)
        throw std::invalid_argument("laplacian_blur(): diffusion must be non-negative");

      grid_type const original = image;

      size_t const I = image.I();
      size_t const J = image.J();
      size_t const K = image.K();

      real_type const nu = static_cast<real_type>(diffusion);
      real_type const cx = nu / ( image.dx() * image.dx() );
      real_type const cy = nu / ( image.dy() * image.dy() );
      real_type const cz = nu / ( image.dz() * image.dz() );
      real_type const inv_diagonal = value_traits::one() / ( value_traits::one() + value_traits::two() * ( cx + cy + cz ) );

      for(size_t iteration = 0; iteration < max_iterations; ++iteration)
      {
        for(size_t k = 0; k < K; ++k)
          for(size_t j = 0; j < J; ++j)
            for(size_t i = 0; i < I; ++i)
            {
              // An index outside the grid is clamped onto the boundary, which makes the
              // normal derivative zero there -- the same convention as poisson_solver.
              size_t const im1 = i ? i - 1 : 0;
              size_t const jm1 = j ? j - 1 : 0;
              size_t const km1 = k ? k - 1 : 0;
              size_t const ip1 = min( i + 1, I - 1 );
              size_t const jp1 = min( j + 1, J - 1 );
              size_t const kp1 = min( k + 1, K - 1 );

              real_type const neighbours =
                  cx * ( real_type( image(ip1, j, k) ) + real_type( image(im1, j, k) ) )
                + cy * ( real_type( image(i, jp1, k) ) + real_type( image(i, jm1, k) ) )
                + cz * ( real_type( image(i, j, kp1) ) + real_type( image(i, j, km1) ) );

              image(i, j, k) = static_cast<value_type>( ( real_type( original(i, j, k) ) + neighbours ) * inv_diagonal );
            }
      }
    }

  } // namespace grid
} // namespace OpenTissue

// OPENTISSUE_CORE_CONTAINERS_GRID_UTIL_GRID_LAPLACIAN_BLUR_H
#endif

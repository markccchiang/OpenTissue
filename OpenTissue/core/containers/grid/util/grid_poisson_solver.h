#ifndef OPENTISSUE_CORE_CONTAINERS_GRID_UTIL_GRID_POISSON_SOLVER_H
#define OPENTISSUE_CORE_CONTAINERS_GRID_UTIL_GRID_POISSON_SOLVER_H
//
// OpenTissue Template Library
// - A generic toolbox for physics-based modeling and simulation.
// Copyright (C) 2008 Department of Computer Science, University of Copenhagen.
//
// OTTL is licensed under zlib: http://opensource.org/licenses/zlib-license.php
//
#include <OpenTissue/configuration.h>

#include <cmath>

#include <OpenTissue/core/containers/grid/util/grid_compute_sign_function.h>
#include <OpenTissue/core/math/math_vector3.h>

namespace OpenTissue
{
  namespace grid
  {

    /**
    * Gauss-Seidel Poisson Solver with Pure Von Neuman Boundary Conditions.
    *
    * Theory: Given the PDE
    *
    * \f[ \nabla^2 \phi = W \f]
    *
    * Solve for phi. Writing out we have
    *
    * \f[ \frac{\partial^2 \phi}{\partial x^2} + \frac{\partial^2 \phi}{\partial y^2} + \frac{\partial^2 \phi}{\partial z^2} = W \f]
    *
    * Using central diff approximation leads to
    *
    * \f[
    *   \phi_{i,j,k} = \frac{ a_2 ( \phi_{i+1,j,k} + \phi_{i-1,j,k} ) + a_1 ( \phi_{i,j+1,k} + \phi_{i,j-1,k} ) + a_0 ( \phi_{i,j,k+1} + \phi_{i,j,k-1} ) - a_3 W }{ 2 a_4 }
    * \f]
    *
    *  where
    *
    * \f{align*}{
    *   a_0 &= \Delta x^2 \Delta y^2, &
    *   a_1 &= \Delta x^2 \Delta z^2, &
    *   a_2 &= \Delta y^2 \Delta z^2, \\
    *   a_3 &= \Delta x^2 \Delta y^2 \Delta z^2, &
    *   a_4 &= a_0 + a_1 + a_2
    * \f}
    *
    *  In case \f$\Delta x = \Delta y = \Delta z\f$ this simplifies to
    *
    * \f[
    *   \phi_{i,j,k} = \frac{ \phi_{i+1,j,k} + \phi_{i-1,j,k} + \phi_{i,j+1,k} + \phi_{i,j-1,k} + \phi_{i,j,k+1} + \phi_{i,j,k-1} - \Delta x^2 W }{ 6 }
    * \f]
    *
    * The solver uses pure Neumann bondary conditions. i.e.:
    *
    * \f[ \nabla \phi = 0 \f]
    *
    * on any boundary. This means that values outside boundary are copied from
    * nearest boundary voxel => claming out-of-bound indices onto boundary.
    *
    * @param phi              Contains initial guess for solution, and upon
    *                         return contains the solution.
    * @param W                The right hand side of the poisson equation.
    * @param max_iterations   The maximum number of iterations allowed. Default is 30 iterations.
    */
    template < typename grid_type >
    inline void poisson_solver(
      grid_type & phi
      , grid_type const & W
      , size_t max_iterations = 10
      )
    {
      using std::min;

      typedef typename grid_type::value_type            value_type;
      typedef typename grid_type::const_iterator        const_iterator;
      typedef typename grid_type::index_iterator        index_iterator;
      typedef typename grid_type::math_types            math_types;
      typedef typename math_types::real_type            real_type;

      size_t I = phi.I();
      size_t J = phi.J();
      size_t K = phi.K();
      real_type dx = phi.dx();
      real_type dy = phi.dy();
      real_type dz = phi.dz();

      if(dx!=dy || dx!=dz || dy!=dz)
      {
        //std::cout << "poisson_solver(): non-uniform grid" << std::endl;

        real_type a0 = dx*dx*dy*dy;
        real_type a1 = dx*dx*dz*dz;
        real_type a2 = dy*dy*dz*dz;
        real_type a3 = dx*dx*dy*dy*dz*dz;
        real_type a4 = 1.0/(2*(a0 + a1 + a2));

        for(size_t iteration=0;iteration<max_iterations;++iteration)
        {
          index_iterator  p      = phi.begin();
          index_iterator  p_end  = phi.end();
          const_iterator  w      = W.begin();
          for(;p!=p_end;++p,++w)
          {
            size_t i = p.i();
            size_t j = p.j();
            size_t k = p.k();
            size_t im1         = ( i ) ?  i - 1 : 0;
            size_t jm1         = ( j ) ?  j - 1 : 0;
            size_t km1         = ( k ) ?  k - 1 : 0;
            size_t ip1         = min( i + 1u, I - 1u );
            size_t jp1         = min( j + 1u, J - 1u );
            size_t kp1         = min( k + 1u, K - 1u );
            //size_t idx         = ( k   * J + j )   * I + i;
            size_t idx_im1     = ( k   * J + j )   * I + im1;
            size_t idx_ip1     = ( k   * J + j )   * I + ip1;
            size_t idx_jm1     = ( k   * J + jm1 ) * I + i;
            size_t idx_jp1     = ( k   * J + jp1 ) * I + i;
            size_t idx_km1     = ( km1 * J + j )   * I + i;
            size_t idx_kp1     = ( kp1 * J + j )   * I + i;
            //real_type  v000  = phi(idx);
            real_type  vm00  = phi(idx_im1);
            real_type  vp00  = phi(idx_ip1);
            real_type  v0m0  = phi(idx_jm1);
            real_type  v0p0  = phi(idx_jp1);
            real_type  v00m  = phi(idx_km1);
            real_type  v00p  = phi(idx_kp1);
            *p =  ( a2*( vp00 + vm00 ) + a1*(v0p0+v0m0) + a0*(v00p + v00m)  - a3*(*w)) * a4;
          }
        }
      }
      else
      {
        //std::cout << "poisson_solver(): uniform grid, dx=dy=dz" << std::endl;

        real_type a0 = dx*dx;
        real_type a1 = 1.0/8.0;
        for(size_t iteration=0;iteration<max_iterations;++iteration)
        {
          index_iterator  p      = phi.begin();
          index_iterator  p_end  = phi.end();
          const_iterator  w      = W.begin();
          for(;p!=p_end;++p,++w)
          {
            size_t i = p.i();
            size_t j = p.j();
            size_t k = p.k();
            size_t im1         = ( i ) ?  i - 1 : 0;
            size_t jm1         = ( j ) ?  j - 1 : 0;
            size_t km1         = ( k ) ?  k - 1 : 0;
            size_t ip1         = min( i + 1u, I - 1u );
            size_t jp1         = min( j + 1u, J - 1u );
            size_t kp1         = min( k + 1u, K - 1u );
            //size_t idx         = ( k   * J + j )   * I + i;
            size_t idx_im1     = ( k   * J + j )   * I + im1;
            size_t idx_ip1     = ( k   * J + j )   * I + ip1;
            size_t idx_jm1     = ( k   * J + jm1 ) * I + i;
            size_t idx_jp1     = ( k   * J + jp1 ) * I + i;
            size_t idx_km1     = ( km1 * J + j )   * I + i;
            size_t idx_kp1     = ( kp1 * J + j )   * I + i;
            //real_type  v000  = phi(idx);
            real_type  vm00  = phi(idx_im1);
            real_type  vp00  = phi(idx_ip1);
            real_type  v0m0  = phi(idx_jm1);
            real_type  v0p0  = phi(idx_jp1);
            real_type  v00m  = phi(idx_km1);
            real_type  v00p  = phi(idx_kp1);
            *p =  ( vp00 + vm00 + v0p0+v0m0 + v00p + v00m  - a0*(*w) ) *a1;
          }
        }
      }
    }

  } // namespace grid
} // namespace OpenTissue

// OPENTISSUE_CORE_CONTAINERS_GRID_UTIL_GRID_POISSON_SOLVER_H
#endif

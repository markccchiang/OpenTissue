#ifndef OPENTISSUE_CORE_MATH_OPTIMIZATION_PROJECTED_BFGS_H
#define OPENTISSUE_CORE_MATH_OPTIMIZATION_PROJECTED_BFGS_H
//
// OpenTissue Template Library
// - A generic toolbox for physics-based modeling and simulation.
// Copyright (C) 2008 Department of Computer Science, University of Copenhagen.
//
// OTTL is licensed under zlib: http://opensource.org/licenses/zlib-license.php
//
#include <OpenTissue/configuration.h>

#include <OpenTissue/core/math/big/big_types.h>
#include <OpenTissue/core/math/optimization/optimization_bfgs.h> // The unprojected version
#include <OpenTissue/core/math/optimization/optimization_constants.h>
#include <OpenTissue/core/math/optimization/optimization_armijo_projected_backtracking.h>
#include <OpenTissue/core/math/optimization/optimization_stationary_point.h>
#include <OpenTissue/core/math/optimization/optimization_absolute_convergence.h>

#include <OpenTissue/core/math/math_value_traits.h>
#include <OpenTissue/core/math/math_is_number.h>

#include <cmath>
#include <stdexcept>
#include <cassert>
#include <vector>

namespace OpenTissue
{
  namespace math
  {
    namespace optimization
    {

      /**
      * A projected BFGS implemention.
      * See the comments for the bfgs method.
      * This implementation was made to solve problems of the type
      *
      * \f[  \vec x^* = \min_{\vec x} f(\vec x)  \text{s.t.}  \vec l \leq x \leq \vec u \f]
      *
      * It does so by performing a projected line-searh. The idea is to rewrite
      * the constraints as a projection operator,
      *
      * \f[  P(\vec x) = \min\left( \vec u, \max\left( \vec x, \vec l\right)    \right)   \f]
      *
      * See the file optimization_project for examples of such re-writes. Next the projection
      * operator is applied during the line-search to make sure that only feasible
      * iterates are generated. See the method armijo_projected_backtracking for
      * details on the line-search performed.
      *
      * @param P      The projection operator to be used. Observe that one
      *               could specify any projection operator. Even one with
      *               variable bounds.
      */
      template < 
        typename T
        , typename function_functor
        , typename gradient_functor
        , typename projection_operator
      >
      inline void projected_bfgs(
        function_functor  & f
      , gradient_functor & nabla_f
      , ublas::compressed_matrix<T>  & H
      , boost::numeric::ublas::vector<T> & x
      , projection_operator const & P
      , size_t const & max_iterations
      , T      const & absolute_tolerance
      , T      const & relative_tolerance
      , T      const & stagnation_tolerance
      , size_t       & status
      , size_t       & iteration
      , T            & error
      , T      const & alpha
      , T      const & beta
      , ublas::vector<T> * profiling = 0
      )  
      {
        using std::fabs;
        using std::min;
        using std::max;

        typedef          ublas::compressed_matrix<T>       matrix_type;
        typedef          ublas::vector<T>                  vector_type;
        typedef          T                                 real_type;
        typedef          OpenTissue::math::ValueTraits<T>  value_traits;

        if(max_iterations <= 0)
          throw std::invalid_argument("max_iterations must be larger than zero");
        if(absolute_tolerance < value_traits::zero() )
          throw std::invalid_argument("absolute_tolerance must be non-negative");
        if(relative_tolerance < value_traits::zero() )
          throw std::invalid_argument("relative_tolerance must be non-negative");
        if(stagnation_tolerance < value_traits::zero() )
          throw std::invalid_argument("stagnation_tolerance must be non-negative");
        if (beta >= value_traits::one() )
          throw std::invalid_argument("Illegal beta value");
        if (alpha <= value_traits::zero() )
          throw std::invalid_argument("Illegal alpha value");
        if(beta<=alpha)
          throw std::invalid_argument("beta must be larger than alpha");
        if(profiling == &x)
          throw std::logic_error("profiling must not point to x-vector");

        error             = value_traits::infinity();
        iteration = 0;

        status = OK;

        size_t const m = x.size();
        if(m==0)
          return;

        status = ITERATING; // Indicate that we are iterating and have not converged

        if(profiling)
        {
          (*profiling).resize( max_iterations );
          (*profiling).clear();
        }

        // Declare temporary storage
        vector_type y_k;
        vector_type s_k;
        vector_type dx;
        vector_type x_old;
        vector_type nabla_f_k;
        vector_type nabla_f_k1;
        vector_type projected_gradient;
        vector_type trial;
        vector_type step;
        vector_type free_gradient;
        std::vector<bool> active;

        // Allocate space for temporaries
        projected_gradient.resize(m);
        trial.resize(m);
        step.resize(m);
        free_gradient.resize(m);
        active.resize(m);
        y_k.resize(m);
        s_k.resize(m);
        dx.resize(m);
        x_old.resize(m);
        nabla_f_k.resize(m);
        nabla_f_k1.resize(m);

        // Initialize 

        x = P(x); // Make sure that the initial x-value is a feasible iterate!
        real_type             f_0   = f(x);
        ublas::noalias( nabla_f_k ) = nabla_f(x);

        // Iterate until convergence
        for (; iteration < max_iterations; ++iteration)
        {
          if(profiling)
            (*profiling)(iteration) = f_0;
        
          // Check for absolute convergence. At a minimizer on a bound the gradient is not
          // zero, it points out of the feasible region, so test the projected gradient
          // x - P(x - nabla f) instead: it is zero exactly at a constrained stationary point.
          ublas::noalias( projected_gradient ) = x - P( vector_type( x - nabla_f_k ) );
          if(stationary_point( projected_gradient, absolute_tolerance, error ) )
          {
            status = ABSOLUTE_CONVERGENCE;
            return;
          }

          // Compute the search direction as in the projected Newton method of Bertsekas
          // (1982). A variable is active if it lies within epsilon of a bound and its
          // gradient pushes it into that bound; epsilon shrinks as the iterates converge.
          // Moving every variable epsilon against its gradient and seeing which ones the
          // projection clips finds exactly those. Active variables take a steepest descent
          // step, which the projection then cancels; the others take the quasi-Newton step
          // computed without the active components. Using -H nabla f for all of them lets the
          // gradient of an active variable, which cannot move, push the free variables away
          // from their optimum through the off-diagonal terms of H.
          real_type const epsilon = min( value_traits::one(), ublas::norm_inf( projected_gradient ) );
          for(size_t i = 0; i < m; ++i)
            step(i) = nabla_f_k(i) > value_traits::zero() ? -epsilon : ( nabla_f_k(i) < value_traits::zero() ? epsilon : value_traits::zero() );
          ublas::noalias( trial ) = P( vector_type( x + step ) );
          for(size_t i = 0; i < m; ++i)
            active[i] = fabs( trial(i) - ( x(i) + step(i) ) ) > value_traits::zero();

          ublas::noalias( free_gradient ) = nabla_f_k;
          for(size_t i = 0; i < m; ++i)
            if(active[i])
              free_gradient(i) = value_traits::zero();

          // ublas::noalias( dx ) = - ublas::prod(H, free_gradient);
          ublas::axpy_prod(H, -free_gradient, dx, true);
          for(size_t i = 0; i < m; ++i)
            if(active[i])
              dx(i) = -nabla_f_k(i);

          // H should stay positive definite, which makes dx a descent direction. Should
          // round-off have spoilt that, restart from H = I (steepest descent).
          bool restarted = false;
          if( ublas::inner_prod(dx, nabla_f_k) >= value_traits::zero() )
          {
            detail::bfgs_reset_inverse_hessian(H);
            ublas::noalias( dx ) = -nabla_f_k;
            restarted = true;
          }

          x_old.assign( x );
          real_type f_tau = f_0;
          armijo_projected_backtracking(
            f
            , nabla_f_k
            , x_old
            , x
            , dx
            , relative_tolerance
            , stagnation_tolerance
            , alpha
            , beta
            , f_tau
            , status
            , P
            );

          // The line-search stops the iteration on stagnation or a small relative change in
          // f. Along a quasi-Newton direction that proves little: a poorly scaled H -- say
          // one that started out singular -- gives a direction too short, or too long and
          // back-tracked to almost nothing, so f barely changes although the projected
          // gradient test above has just failed. Before accepting such a stop, retry once
          // along the steepest descent direction; if that stalls as well, the stop stands.
          if( status != OK && !restarted )
          {
            detail::bfgs_reset_inverse_hessian(H);
            ublas::noalias( dx ) = -nabla_f_k;
            x.assign( x_old );
            f_tau = f_0;
            armijo_projected_backtracking(
              f
              , nabla_f_k
              , x_old
              , x
              , dx
              , relative_tolerance
              , stagnation_tolerance
              , alpha
              , beta
              , f_tau
              , status
              , P
              );
          }

          if(status != OK ) 
            return;
                    
          // Do the incremental update of the inverse Hessian approximation
          ublas::noalias( nabla_f_k1 ) = nabla_f(x);
          ublas::noalias( s_k )  = x - x_old;
          ublas::noalias( y_k ) = nabla_f_k1 - nabla_f_k;

          detail::bfgs_update_inverse_hessian(y_k,s_k,H);

          // Update values for next iteration
          f_0 = f_tau;
          nabla_f_k.assign( nabla_f_k1 );

        }//end for loop
      }

    } // namespace optimization
  } // namespace math
} // namespace OpenTissue

// OPENTISSUE_CORE_MATH_OPTIMIZATION_PROJECTED_BFGS_H
#endif

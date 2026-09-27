//
// OpenTissue, A toolbox for physical based simulation and animation.
// Copyright (C) 2007 Department of Computer Science, University of Copenhagen
//
#include <OpenTissue/configuration.h>

#include <OpenTissue/core/math/big/big_types.h>
#include <OpenTissue/core/math/optimization/optimization_armijo_backtracking.h>
#include <OpenTissue/core/math/optimization/optimization_armijo_projected_backtracking.h>
#include <OpenTissue/core/math/optimization/optimization_constants.h>

#include <algorithm>

#define BOOST_AUTO_TEST_MAIN
#include <OpenTissue/utility/utility_push_boost_filter.h>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>
#include <boost/test/tools/floating_point_comparison.hpp>
#include <boost/test/test_tools.hpp>
#include <OpenTissue/utility/utility_pop_boost_filter.h>

typedef double                   real_type;
typedef ublas::vector<real_type> vector_type;

using OpenTissue::math::optimization::armijo_backtracking;
using OpenTissue::math::optimization::armijo_projected_backtracking;
using OpenTissue::math::optimization::OK;
using OpenTissue::math::optimization::BACKTRACKING_FAILED;

/**
* f(x) = x^T x.
*/
class Bowl
{
public:
  real_type operator()( vector_type const & x ) const { return ublas::inner_prod(x, x); }
};

/**
* Projection onto a box so large it never clips: the projected search then behaves like
* the plain one.
*/
class NoBounds
{
public:
  vector_type operator()( vector_type const & x ) const { return x; }
};

vector_type make_vector(real_type a, real_type b)
{
  vector_type v(2);
  v(0) = a;
  v(1) = b;
  return v;
}

BOOST_AUTO_TEST_SUITE(opentissue_math_big_armijo_backtracking);

// A gradient with the wrong sign makes an uphill direction look like a descent direction,
// so every trial step increases f and the search must fail. It then used to hand back its
// last, rejected, trial point -- higher up than where it started -- and, with a loose
// relative tolerance, to report "relative convergence" instead of the failure, since the
// rejected point differs from the start by a tiny step.

BOOST_AUTO_TEST_CASE(failed_search_restores_the_start)
{
  Bowl              f;
  vector_type const x         = make_vector( 1.0, -2.0 );
  vector_type const wrong_g   = -2.0 * x;          // the true gradient is 2 x
  vector_type const dx        = -wrong_g;          // "descent" for the wrong gradient: uphill
  real_type   const f_start   = f(x);

  vector_type x_tau(2);
  real_type   f_tau  = f_start;
  size_t      status = OK;
  armijo_backtracking( f, wrong_g, x, x_tau, dx, 1e-3, 1e-9, 0.0001, 0.5, f_tau, status );

  BOOST_CHECK_EQUAL( status, BACKTRACKING_FAILED );
  BOOST_CHECK_EQUAL( x_tau(0), x(0) );
  BOOST_CHECK_EQUAL( x_tau(1), x(1) );
  BOOST_CHECK_EQUAL( f_tau, f_start );
}

BOOST_AUTO_TEST_CASE(failed_projected_search_restores_the_start)
{
  Bowl              f;
  NoBounds          P;
  vector_type const x         = make_vector( 1.0, -2.0 );
  vector_type const wrong_g   = -2.0 * x;
  vector_type const dx        = -wrong_g;
  real_type   const f_start   = f(x);

  vector_type x_tau(2);
  real_type   f_tau  = f_start;
  size_t      status = OK;
  armijo_projected_backtracking( f, wrong_g, x, x_tau, dx, 1e-3, 1e-9, 0.0001, 0.5, f_tau, status, P );

  BOOST_CHECK_EQUAL( status, BACKTRACKING_FAILED );
  BOOST_CHECK_EQUAL( x_tau(0), x(0) );
  BOOST_CHECK_EQUAL( x_tau(1), x(1) );
  BOOST_CHECK_EQUAL( f_tau, f_start );
}

// Whatever step a search accepts must satisfy the Armijo condition it is named after:
//
//   f(x_tau) <= f(x) + alpha nabla f(x)^T (x_tau - x)
//
// Here the full step overshoots the minimizer of f(x) = x^T x from x = [1, 0] to [-2, 0]. The
// projected search used to multiply the right-hand side by tau once more than it should,
// which accepted tau = 1/2 although it gives too little decrease; tau = 1/4 is the first
// step that passes.

template<typename search_functor>
void check_accepted_step_satisfies_armijo(search_functor search)
{
  Bowl              f;
  real_type   const alpha = 0.4;  // the searches require alpha < beta = 0.5
  vector_type const x     = make_vector( 1.0, 0.0 );
  vector_type const g     = 2.0 * x;
  vector_type const dx    = make_vector( -3.0, 0.0 );

  vector_type x_tau(2);
  real_type   f_tau  = f(x);
  size_t      status = OK;
  real_type const tau = search( f, g, x, x_tau, dx, alpha, f_tau, status );

  BOOST_CHECK_EQUAL( status, OK );
  BOOST_CHECK( f(x_tau) <= f(x) + alpha * ublas::inner_prod( g, vector_type( x_tau - x ) ) );
  BOOST_CHECK_CLOSE( tau, 0.25, 1e-9 );
}

struct PlainSearch
{
  real_type operator()( Bowl const & f, vector_type const & g, vector_type const & x, vector_type & x_tau, vector_type const & dx, real_type alpha, real_type & f_tau, size_t & status ) const
  {
    return armijo_backtracking( f, g, x, x_tau, dx, 0.0, 0.0, alpha, 0.5, f_tau, status );
  }
};

struct ProjectedSearch
{
  real_type operator()( Bowl const & f, vector_type const & g, vector_type const & x, vector_type & x_tau, vector_type const & dx, real_type alpha, real_type & f_tau, size_t & status ) const
  {
    return armijo_projected_backtracking( f, g, x, x_tau, dx, 0.0, 0.0, alpha, 0.5, f_tau, status, NoBounds() );
  }
};

BOOST_AUTO_TEST_CASE(accepted_step_satisfies_armijo_condition)
{
  check_accepted_step_satisfies_armijo( PlainSearch() );
}

BOOST_AUTO_TEST_CASE(accepted_projected_step_satisfies_armijo_condition)
{
  check_accepted_step_satisfies_armijo( ProjectedSearch() );
}

BOOST_AUTO_TEST_SUITE_END();

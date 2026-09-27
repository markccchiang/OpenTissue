//
// OpenTissue, A toolbox for physical based simulation and animation.
// Copyright (C) 2007 Department of Computer Science, University of Copenhagen
//
#include <OpenTissue/configuration.h>

#include <OpenTissue/core/math/big/big_types.h>
#include <OpenTissue/core/math/optimization/optimization_projected_bfgs.h>
#include <OpenTissue/core/math/big/big_generate_random.h>
#include <OpenTissue/core/math/big/big_generate_PD.h>
#include <OpenTissue/core/math/big/io/big_matlab_write.h>
#include <OpenTissue/core/math/optimization/optimization_project.h>

#define BOOST_AUTO_TEST_MAIN
#include <OpenTissue/utility/utility_push_boost_filter.h>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>
#include <boost/test/tools/floating_point_comparison.hpp>
#include <boost/test/test_tools.hpp>
#include <OpenTissue/utility/utility_pop_boost_filter.h>

typedef double real_type;
typedef ublas::compressed_matrix<real_type> matrix_type;
typedef ublas::vector<real_type>            vector_type;
typedef vector_type::size_type              size_type;

class F
{
public:
  matrix_type const & m_A;
  vector_type const & m_b;

  F(matrix_type const & A, vector_type const & b)
    : m_A(A)
    , m_b(b)
  {}

  real_type operator()( vector_type const & x ) const
  {
    return ublas::inner_prod(x, ublas::prod(m_A,x)) - inner_prod(m_b, x);
  }
};

class nabla_F
{
public:
  matrix_type const & m_A;
  vector_type const & m_b;

  nabla_F(matrix_type const & A, vector_type const & b)
    : m_A(A)
    , m_b(b)
  {}

  vector_type operator()( vector_type const & x ) const
  {
    return  vector_type( 2*ublas::prod(m_A,x) - m_b );
  }
};

class ProjectionOperator
{
public:
  vector_type const & m_l;
  vector_type const & m_u;

  ProjectionOperator(vector_type const & l, vector_type const & u)
    : m_l(l)
    , m_u(u)
  {}

  vector_type operator()( vector_type const & x ) const
  {
    vector_type y;
    y.resize(x.size());
    OpenTissue::math::optimization::project(x,m_l,m_u,y);
    return  y;
  }
};




// Whether to print each solve. Off while sweeping many seeds, where it would bury the
// output of a failure.
bool verbose = true;

void do_test(F & f, nabla_F & nabla_f, vector_type & x, matrix_type & H, ProjectionOperator & P, vector_type const & y)
{
  using namespace OpenTissue::math::big;

  // The solver starts from the projection of x onto the feasible set.
  real_type const f_start = f( P(x) );

  size_type max_iterations       = 100;
  real_type absolute_tolerance   = boost::numeric_cast<real_type>(1e-6);
  real_type relative_tolerance   = boost::numeric_cast<real_type>(0.000000001);
  real_type stagnation_tolerance = boost::numeric_cast<real_type>(0.000000001);
  size_t status = 0;
  size_type iteration = 0;
  real_type accuracy = boost::numeric_cast<real_type>(0.0);
  real_type alpha = boost::numeric_cast<real_type>(0.0001);
  real_type beta = boost::numeric_cast<real_type>(0.5);

  OpenTissue::math::optimization::projected_bfgs(
    f
    , nabla_f
    , H
    , x 
    , P
    , max_iterations
    , absolute_tolerance
    , relative_tolerance
    , stagnation_tolerance
    , status
    , iteration
    , accuracy
    , alpha
    , beta
    );

  if(verbose) std::cout << "status     = " 
    << OpenTissue::math::optimization::get_error_message(status) 
    << std::endl;
  if(verbose) std::cout << "absolute   = " 
    << accuracy  
    << std::endl;
  if(verbose) std::cout << "iterations = " 
    << iteration 
    << std::endl;
  if(verbose) std::cout << "x          = " 
    << x 
    << std::endl;

  // Whatever else happens, the solver must not hand back a point worse than it was given.
  BOOST_CHECK( f(x) <= f_start );

  if(status==OpenTissue::math::optimization::ABSOLUTE_CONVERGENCE)
  {
    BOOST_CHECK( accuracy < absolute_tolerance );
    BOOST_CHECK( iteration <= max_iterations );
  }

  // In percent. The solver may stop on its relative test, when f changes by less than
  // relative_tolerance (1e-9) between iterations. Near a minimum f is quadratic in the
  // distance to it, so that leaves x up to about sqrt(1e-9) away -- more than the 0.001% this
  // check once demanded, which failed a correct solver on some random starting points.
  double tol = 0.01;
  for(size_t i = 0;i<x.size();++i)
    BOOST_CHECK_CLOSE( x(i), y(i), tol);
}

BOOST_AUTO_TEST_SUITE(opentissue_math_big_projected_bfgs);

void unconstrained_scenarios()
{
  using namespace OpenTissue::math::big;

  // We are solving the problem
  //
  //   min_x Q(x) = x^T A x - b^T x
  //
  // where the gradient is given by
  //
  //   nabla Q(x) = 2 A x - b = 0
  //
  // and the exact Hessian is
  //
  //   H = nabla^2 Q(x) = 2 A
  //
  // The stationary points are given by 
  //
  //  | 4 0| |x_1| + | -1| = 0
  //  | 0 4| |x_2|   | -2|
  //
  // and has the unique solution x = [-0.25, -0.5]^T
  //
  size_type N = 2;

  matrix_type A;
  A.resize(N,N,false);

  vector_type b;
  b.resize(N,false);

  A(0,0) = 2.0;  A(0,1) = 0.0;
  A(1,0) = 0.0;  A(1,1) = 2.0;  

  b(0) = -1.0;
  b(1) = -2.0;

  F f(A,b);
  nabla_F nabla_f(A,b);

  vector_type x;
  x.resize(N,false);

  vector_type y;
  y.resize(N,false);
  y(0) = -0.25;
  y(1) = -0.5;

  matrix_type H;
  H.resize(N,N,false);

  vector_type l;
  vector_type u;

  l.resize(N,false);
  u.resize(N,false);

  l(0) = -1.0;
  l(1) = -1.0;
  u(0) =  1.0;
  u(1) =  1.0;

  ProjectionOperator P(l,u);

  // use H = I/4, and x = 0
  x.clear();
  H(0,0) = 0.25;   H(0,1) = 0.0;
  H(1,0) = 0.0;    H(1,1) = 0.25;   
  do_test(f,nabla_f,x,H,P,y);

  // use H = I, and x = 0
  x.clear();
  H(0,0) = 1.0;   H(0,1) = 0.0;
  H(1,0) = 0.0;    H(1,1) = 1.0;   
  do_test(f,nabla_f,x,H,P,y);

  // use H = 4*I, and x = 0
  x.clear();
  H(0,0) = 4.0;   H(0,1) = 0.0;
  H(1,0) = 0.0;    H(1,1) = 4.0;   
  do_test(f,nabla_f,x,H,P,y);

  // use H = I/4, and x = random
  OpenTissue::math::big::generate_random( 2, x);
  H(0,0) = 0.25;   H(0,1) = 0.0;
  H(1,0) = 0.0;    H(1,1) = 0.25;   
  do_test(f,nabla_f,x,H,P,y);

  // use H = I, and x = random
  OpenTissue::math::big::generate_random( 2, x);
  H(0,0) = 1.0;   H(0,1) = 0.0;
  H(1,0) = 0.0;    H(1,1) = 1.0;   
  do_test(f,nabla_f,x,H,P,y);

  // use H = 4*I, and x = random
  OpenTissue::math::big::generate_random( 2, x);
  H(0,0) = 4.0;   H(0,1) = 0.0;
  H(1,0) = 0.0;    H(1,1) = 4.0;   
  do_test(f,nabla_f,x,H,P,y);

  // use H = random PD, and x = 0
  x.clear();
  OpenTissue::math::big::generate_PD(2, H);
  do_test(f,nabla_f,x,H,P,y);

  // use H = random PD, and x = random
  OpenTissue::math::big::generate_random( 2, x);
  OpenTissue::math::big::generate_PD(2, H);
  do_test(f,nabla_f,x,H,P,y);

  // H = g g^T, x = 0
  x.clear();
  vector_type g;
  g.resize(N,false);
  g = nabla_f(x);
  H = ublas::outer_prod(g,g);
  do_test(f,nabla_f,x,H,P,y);

  // H = g g^T, x = random
  OpenTissue::math::big::generate_random( 2, x);
  g = nabla_f(x);
  H = ublas::outer_prod(g,g);
  do_test(f,nabla_f,x,H,P,y);

  // H = exact Hessian, x = solution!
  x(0) = -0.25;
  x(1) = -0.5;
  H(0,0) = 4.0;   H(0,1) = 0.0;
  H(1,0) = 0.0;    H(1,1) = 4.0;   
  do_test(f,nabla_f,x,H,P,y);

}


void constrained_scenarios()
{
  using namespace OpenTissue::math::big;

  // We are solving the problem
  //
  //   min_x Q(x) = x^T A x - b^T x
  //
  // where the gradient is given by
  //
  //   nabla Q(x) = 2 A x - b = 0
  //
  // and the exact Hessian is
  //
  //   H = nabla^2 Q(x) = 2 A
  //
  // The stationary points are given by 
  //
  //  | 4 0| |x_1| + | -1| = 0
  //  | 0 4| |x_2|   | -2|
  //
  // and has the unique solution x = [-0.25, -0.5]^T
  //
  size_type N = 2;

  matrix_type A;
  A.resize(N,N,false);

  vector_type b;
  b.resize(N,false);

  A(0,0) = 2.0;  A(0,1) = 0.0;
  A(1,0) = 0.0;  A(1,1) = 2.0;  

  b(0) = -1.0;
  b(1) = -2.0;

  F f(A,b);
  nabla_F nabla_f(A,b);

  vector_type x;
  x.resize(N,false);

  vector_type l;
  vector_type u;
  l.resize(N,false);
  u.resize(N,false);
  l(0) = 0.0;
  l(1) = -1.0;
  u(0) =  1.0;
  u(1) =  1.0;

  vector_type y;
  y.resize(N,false);
  y(0) = 0.0;
  y(1) = -0.5;

  matrix_type H;
  H.resize(N,N,false);

  ProjectionOperator P(l,u);

  // use H = I/4, and x = 0
  x.clear();
  H(0,0) = 0.25;   H(0,1) = 0.0;
  H(1,0) = 0.0;    H(1,1) = 0.25;   
  do_test(f,nabla_f,x,H,P,y);

  // use H = I, and x = 0
  x.clear();
  H(0,0) = 1.0;   H(0,1) = 0.0;
  H(1,0) = 0.0;    H(1,1) = 1.0;   
  do_test(f,nabla_f,x,H,P,y);

  // use H = 4*I, and x = 0
  x.clear();
  H(0,0) = 4.0;   H(0,1) = 0.0;
  H(1,0) = 0.0;    H(1,1) = 4.0;   
  do_test(f,nabla_f,x,H,P,y);

  // use H = I/4, and x = random
  OpenTissue::math::big::generate_random( 2, x);
  H(0,0) = 0.25;   H(0,1) = 0.0;
  H(1,0) = 0.0;    H(1,1) = 0.25;   
  do_test(f,nabla_f,x,H,P,y);

  // use H = I, and x = random
  OpenTissue::math::big::generate_random( 2, x);
  H(0,0) = 1.0;   H(0,1) = 0.0;
  H(1,0) = 0.0;    H(1,1) = 1.0;   
  do_test(f,nabla_f,x,H,P,y);

  // use H = 4*I, and x = random
  OpenTissue::math::big::generate_random( 2, x);
  H(0,0) = 4.0;   H(0,1) = 0.0;
  H(1,0) = 0.0;    H(1,1) = 4.0;   
  do_test(f,nabla_f,x,H,P,y);

  // use H = random PD, and x = 0
  x.clear();
  OpenTissue::math::big::generate_PD(2, H);
  do_test(f,nabla_f,x,H,P,y);

  // use H = random PD, and x = random
  OpenTissue::math::big::generate_random( 2, x);
  OpenTissue::math::big::generate_PD(2, H);
  do_test(f,nabla_f,x,H,P,y);

  // H = g g^T, x = 0
  x.clear();
  vector_type g;
  g.resize(N,false);
  g = nabla_f(x);
  H = ublas::outer_prod(g,g);
  do_test(f,nabla_f,x,H,P,y);

  // H = g g^T, x = random
  OpenTissue::math::big::generate_random( 2, x);
  g = nabla_f(x);
  H = ublas::outer_prod(g,g);
  do_test(f,nabla_f,x,H,P,y);

  // H = exact Hessian, x = solution!
  x(0) = -0.25;
  x(1) = -0.5;
  H(0,0) = 4.0;   H(0,1) = 0.0;
  H(1,0) = 0.0;    H(1,1) = 4.0;   
  do_test(f,nabla_f,x,H,P,y);

}

BOOST_AUTO_TEST_CASE(unconstrained_global_minimizer)
{
  unconstrained_scenarios();
}

BOOST_AUTO_TEST_CASE(constrained_global_minimizer)
{
  constrained_scenarios();
}

BOOST_AUTO_TEST_CASE(bound_minimizer_with_coupled_hessian_approximation)
{
  // min x^T A x - b^T x with A = 2I and b = [-1, -2], over 0 <= x_0 <= 1, -1 <= x_1 <= 1.
  // The minimizer [0, -0.5] lies on the bound x_0 = 0, where the gradient is [1, 0]: it
  // keeps pushing x_0 into the bound. With an approximation H that couples the variables,
  // -H nabla f turns that push into a step in x_1 as well, and the solver once stalled
  // there with x_1 off its optimum. It must leave the pinned variable out of the
  // quasi-Newton step.
  size_type N = 2;

  matrix_type A;
  A.resize(N,N,false);
  A(0,0) = 2.0;  A(0,1) = 0.0;
  A(1,0) = 0.0;  A(1,1) = 2.0;

  vector_type b;
  b.resize(N,false);
  b(0) = -1.0;
  b(1) = -2.0;

  F f(A,b);
  nabla_F nabla_f(A,b);

  vector_type l, u;
  l.resize(N,false);
  u.resize(N,false);
  l(0) =  0.0;  u(0) = 1.0;
  l(1) = -1.0;  u(1) = 1.0;
  ProjectionOperator P(l,u);

  vector_type y;
  y.resize(N,false);
  y(0) =  0.0;
  y(1) = -0.5;

  // Starting approximations and points for which the solver used to stop short; found by
  // a search over H and x. Each H is symmetric positive definite.
  struct Start { real_type h00, h01, h11, x0, x1; };
  Start const starts[] = {
      { 0.1, -0.09, 0.1,  0.3, -0.5 }
    , { 0.1, -0.06, 0.1,  0.7, -1.0 }
    , { 0.1, -0.03, 0.1,  1.0, -1.0 }
  };
  for(size_t s = 0; s < sizeof(starts)/sizeof(starts[0]); ++s)
  {
    BOOST_TEST_CONTEXT("start " << s)
    {
      matrix_type H;
      H.resize(N,N,false);
      H(0,0) = starts[s].h00;  H(0,1) = starts[s].h01;
      H(1,0) = starts[s].h01;  H(1,1) = starts[s].h11;

      vector_type x;
      x.resize(N,false);
      x(0) = starts[s].x0;
      x(1) = starts[s].x1;

      do_test(f,nabla_f,x,H,P,y);
    }
  }
}

// A gradient functor with the wrong sign, so that every direction the solver computes
// points uphill and every line-search fails.
class wrong_nabla_F
{
public:
  nabla_F const & m_nabla_f;

  wrong_nabla_F(nabla_F const & nabla_f)
    : m_nabla_f(nabla_f)
  {}

  vector_type operator()( vector_type const & x ) const
  {
    return vector_type( -m_nabla_f(x) );
  }
};

BOOST_AUTO_TEST_CASE(never_returns_a_worse_point)
{
  // A failed line-search leaves its last, rejected, trial point behind. The solver used to
  // return it, higher up than where it started. With a wrong gradient no step can succeed,
  // so the solver must give up where it began.
  size_type N = 2;

  matrix_type A;
  A.resize(N,N,false);
  A(0,0) = 2.0;  A(0,1) = 0.0;
  A(1,0) = 0.0;  A(1,1) = 2.0;

  vector_type b;
  b.resize(N,false);
  b(0) = -1.0;
  b(1) = -2.0;

  F             f(A,b);
  nabla_F       nabla_f(A,b);
  wrong_nabla_F wrong(nabla_f);

  vector_type l, u;
  l.resize(N,false);
  u.resize(N,false);
  l(0) = -1.0;  u(0) = 1.0;
  l(1) = -1.0;  u(1) = 1.0;
  ProjectionOperator P(l,u);

  vector_type x;
  x.resize(N,false);
  x(0) =  0.3;
  x(1) = -0.2;
  vector_type const x_start = x;

  matrix_type H;
  H.resize(N,N,false);
  H(0,0) = 1.0;
  H(1,1) = 1.0;

  size_t    status    = 0;
  size_type iteration = 0;
  real_type accuracy  = 0.0;
  OpenTissue::math::optimization::projected_bfgs( f, wrong, H, x, P, 100u, 1e-6, 1e-9, 1e-9, status, iteration, accuracy, 0.0001, 0.5 );

  BOOST_CHECK( status != OpenTissue::math::optimization::ABSOLUTE_CONVERGENCE );
  BOOST_CHECK( f(x) <= f(x_start) );
  BOOST_CHECK_EQUAL( x(0), x_start(0) );
  BOOST_CHECK_EQUAL( x(1), x_start(1) );
}

// The scenarios above draw random starting points and matrices, and CI runs them for one
// pinned seed only. Projected BFGS once failed for about 10% of seeds -- mostly with the
// minimizer on a bound -- while that seed happened to pass. So also run them for a fixed
// range of seeds, reseeding the generator here, independent of the environment.
BOOST_AUTO_TEST_CASE(many_random_starts)
{
  verbose = false;
  for(unsigned int seed = 1u; seed <= 100u; ++seed)
  {
    BOOST_TEST_CONTEXT("seed " << seed)
    {
      OpenTissue::math::Random<real_type>::seed(seed);
      unconstrained_scenarios();
      constrained_scenarios();
    }
  }
  verbose = true;
}

BOOST_AUTO_TEST_SUITE_END();

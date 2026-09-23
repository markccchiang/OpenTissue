//
// OpenTissue, A toolbox for physical based simulation and animation.
// Copyright (C) 2007 Department of Computer Science, University of Copenhagen
//
#include <OpenTissue/configuration.h>

#include <OpenTissue/dynamics/mbd/math/mbd_default_math_policy.h>
#include <OpenTissue/dynamics/mbd/math/mbd_optimized_ublas_math_policy.h>
#include <OpenTissue/dynamics/mbd/mbd.h>

#define BOOST_AUTO_TEST_MAIN
#include <OpenTissue/utility/utility_push_boost_filter.h>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>
#include <boost/test/tools/floating_point_comparison.hpp>
#include <boost/test/test_tools.hpp>
#include <OpenTissue/utility/utility_pop_boost_filter.h>

#include <cmath>
#include <vector>

//
// The two math policies are meant to be interchangeable: the default one assembles the
// system matrix A = J W J^T, the optimized one never forms it and works from J and W J^T
// instead. The compile tests check that a simulator can be assembled on each; these check
// that it then does the same thing.
//
// Two kinds of check, because each misses what the other catches:
//
//   * Known values. The mass-matrix assembly is shared by both policies, so comparing
//     them against each other cannot catch an indexing mistake there -- both would make
//     it. Those matrices are checked entry by entry against the masses and inertias put in.
//
//   * Equivalence. A real simulator is stepped on each policy -- a box swinging on a ball
//     joint from a fixed anchor, under gravity, above a ground box: the ball-joint scene from
//     the original multibody demo -- and the trajectories are compared step by step.
//

namespace
{

  template<typename types>
  class Stepper
    : public OpenTissue::mbd::DynamicsStepper< types, OpenTissue::mbd::ProjectedGaussSeidel<typename types::math_policy> >
  {};

  template<typename types>
  class Detection
    : public OpenTissue::mbd::CollisionDetection<
        types
      , OpenTissue::mbd::SpatialHashing
      , OpenTissue::mbd::GeometryDispatcher
      , OpenTissue::mbd::SingleGroupAnalysis
      >
  {};

  template<typename math_policy_type>
  struct Setup
  {
    typedef OpenTissue::mbd::Types<
        math_policy_type
      , OpenTissue::mbd::NoSleepyPolicy
      , Stepper
      , Detection
      , OpenTissue::mbd::ExplicitFixedStepSimulator
      >                                                         types;

    typedef typename types::simulator_type                      simulator_type;
    typedef typename types::body_type                           body_type;
    typedef typename types::socket_type                         socket_type;
    typedef typename types::configuration_type                  configuration_type;
    typedef typename types::material_library_type               material_library_type;
    typedef typename types::material_type                       material_type;
    typedef typename math_policy_type::real_type                real_type;
    typedef typename math_policy_type::vector3_type             vector3_type;
    typedef typename math_policy_type::quaternion_type          quaternion_type;
    typedef typename math_policy_type::matrix3x3_type           matrix3x3_type;
    typedef typename math_policy_type::coordsys_type            coordsys_type;
    typedef typename math_policy_type::matrix_type              matrix_type;
    typedef OpenTissue::mbd::Gravity<types>                     gravity_type;
    typedef OpenTissue::mbd::BallJoint<types>                   ball_type;
    typedef OpenTissue::geometry::OBB<math_policy_type>         box_type;
  };

  /**
  * The ball-joint scene from the original multibody demo
  * (dc6f0aa^:demos/opengl/glut/multibody/src/scenes/setup_ball_joint.cpp), without its reach
  * cone and with friction and bounce off -- the point is the joint and the linear algebra
  * behind it, not the contact model.
  */
  template<typename math_policy_type>
  class Pendulum
    : public Setup<math_policy_type>
  {
  public:

    typedef Setup<math_policy_type>  setup;

    // Declaration order is destruction order reversed, and it matters here. The configuration
    // refers to the bodies and to the joint, and tears them down in its own destructor -- so
    // everything it refers to has to be declared before it and so outlive it. The joint in
    // turn refers to the sockets, and the bodies to their geometry.
    typename setup::gravity_type           m_gravity;
    typename setup::box_type               m_boxes[2];
    std::vector<typename setup::body_type> m_bodies;
    typename setup::socket_type            m_socket_A;
    typename setup::socket_type            m_socket_B;
    typename setup::ball_type              m_ball;
    typename setup::material_library_type  m_library;
    typename setup::configuration_type     m_configuration;
    typename setup::simulator_type         m_simulator;

    Pendulum()
      : m_bodies(3)
    {
      typedef typename setup::vector3_type    vector3_type;
      typedef typename setup::quaternion_type quaternion_type;
      typedef typename setup::matrix3x3_type  matrix3x3_type;
      typedef typename setup::coordsys_type   coordsys_type;

      double const timestep = 0.01;

      matrix3x3_type const R = OpenTissue::math::diag(typename setup::real_type(1));
      m_boxes[0].set(vector3_type(0, 0, 0), R, vector3_type(30, 1, 30));
      m_boxes[1].set(vector3_type(0, 0, 0), R, vector3_type(2, .5, .5));

      // Ground.
      m_bodies[0].set_position(vector3_type(0, 0, 0));
      m_bodies[0].set_orientation(quaternion_type(1, 0, 0, 0));
      m_bodies[0].set_geometry(&m_boxes[0]);
      m_bodies[0].set_fixed(true);
      m_configuration.add(&m_bodies[0]);

      // Anchor.
      m_bodies[1].attach(&m_gravity);
      m_bodies[1].set_position(vector3_type(-2, 7, 0));
      m_bodies[1].set_orientation(quaternion_type(1, 0, 0, 0));
      m_bodies[1].set_geometry(&m_boxes[1]);
      m_bodies[1].set_fixed(true);
      m_configuration.add(&m_bodies[1]);

      // The swinging box, pushed sideways so the motion is three-dimensional.
      m_bodies[2].attach(&m_gravity);
      m_bodies[2].set_position(vector3_type(2, 7, 0));
      m_bodies[2].set_orientation(quaternion_type(1, 0, 0, 0));
      m_bodies[2].set_velocity(vector3_type(0, 0, 1));
      m_bodies[2].set_geometry(&m_boxes[1]);
      m_bodies[2].set_fixed(false);
      m_configuration.add(&m_bodies[2]);

      m_socket_A.init(m_bodies[1], coordsys_type(vector3_type( 2, 0, 0), quaternion_type()));
      m_socket_B.init(m_bodies[2], coordsys_type(vector3_type(-2, 0, 0), quaternion_type()));
      m_ball.connect(m_socket_A, m_socket_B);
      m_ball.set_frames_per_second(1.0 / timestep);
      m_ball.set_error_reduction_parameter(0.8);
      m_configuration.add(&m_ball);

      m_gravity.set_acceleration(vector3_type(0, -9.81, 0));
      m_simulator.init(m_configuration);
      m_configuration.set_material_library(m_library);

      m_simulator.get_stepper()->get_solver()->set_max_iterations(30);
      m_simulator.get_stepper()->warm_starting()     = false;
      m_simulator.get_stepper()->use_stabilization() = true;
      m_simulator.get_stepper()->use_friction()      = false;
      m_simulator.get_stepper()->use_bounce()        = false;
    }

    typename setup::vector3_type position(size_t i) const
    {
      typename setup::vector3_type r;
      m_bodies[i].get_position(r);
      return r;
    }

  private:

    Pendulum(Pendulum const &);
    Pendulum & operator=(Pendulum const &);
  };

  // Reading back one entry. Through a const reference, because the non-const operator() of a
  // uBLAS sparse matrix inserts the element it is asked for.
  template<typename T>
  double entry(boost::numeric::ublas::compressed_matrix<T> const & M, size_t i, size_t j) { return M(i, j); }

  /**
  * Build the mass and inverse mass matrices for two bodies of known mass and inertia, and
  * check every entry.
  */
  template<typename math_policy_type>
  void check_mass_matrices()
  {
    typedef Setup<math_policy_type>  setup;
    typedef typename setup::vector3_type    vector3_type;
    typedef typename setup::quaternion_type quaternion_type;
    typedef typename setup::matrix3x3_type  matrix3x3_type;

    // Bodies before the configuration, so that it is destroyed first: the configuration
    // refers to the bodies, and tearing them down under it trips its bookkeeping asserts.
    std::vector<typename setup::body_type> bodies(2);
    typename setup::configuration_type configuration;

    double const mass[2]    = { 2.0, 5.0 };
    double const inertia[2][3] = { { 1.0, 2.0, 4.0 }, { 3.0, 6.0, 8.0 } };

    for(size_t b = 0; b < 2; ++b)
    {
      bodies[b].set_position(vector3_type(double(b), 0, 0));
      bodies[b].set_orientation(quaternion_type(1, 0, 0, 0));   // world frame == body frame
      bodies[b].set_mass(mass[b]);
      matrix3x3_type I = OpenTissue::math::diag(typename setup::real_type(0));
      I(0,0) = inertia[b][0];  I(1,1) = inertia[b][1];  I(2,2) = inertia[b][2];
      bodies[b].set_inertia_bf(I);
      configuration.add(&bodies[b]);
    }

    typename setup::matrix_type M;
    typename setup::matrix_type invM;
    OpenTissue::mbd::get_mass_matrix(configuration.body_begin(), configuration.body_end(), M);
    OpenTissue::mbd::get_inverse_mass_matrix(configuration.body_begin(), configuration.body_end(), invM);

    // Bodies are tagged in the order they are visited, so compute where each one's block is
    // from its tag rather than assuming insertion order.
    for(size_t b = 0; b < 2; ++b)
    {
      size_t const o = 6 * bodies[b].m_tag;
      for(size_t r = 0; r < 12; ++r)
        for(size_t c = 0; c < 12; ++c)
        {
          double expected_M    = 0.0;
          double expected_invM = 0.0;
          bool const in_block = r >= o && r < o + 6 && c >= o && c < o + 6;
          if(in_block && r == c)
          {
            size_t const k = r - o;
            expected_M    = k < 3 ? mass[b]       : inertia[b][k - 3];
            expected_invM = k < 3 ? 1.0 / mass[b] : 1.0 / inertia[b][k - 3];
          }
          if(!in_block)
            continue;   // the other body's block is checked on its own iteration
          BOOST_CHECK_SMALL(entry(M,    r, c) - expected_M,    1e-12);
          BOOST_CHECK_SMALL(entry(invM, r, c) - expected_invM, 1e-12);
        }
    }

    // And nothing outside the two diagonal blocks.
    for(size_t r = 0; r < 12; ++r)
      for(size_t c = 0; c < 12; ++c)
        if(r / 6 != c / 6)
        {
          BOOST_CHECK_SMALL(entry(M,    r, c), 1e-12);
          BOOST_CHECK_SMALL(entry(invM, r, c), 1e-12);
        }
  }

  /**
  * Step the pendulum and record the swinging box's position after every step.
  */
  template<typename math_policy_type>
  std::vector<double> trajectory(size_t const steps)
  {
    Pendulum<math_policy_type> scene;
    std::vector<double> path;
    for(size_t s = 0; s < steps; ++s)
    {
      scene.m_simulator.run(0.01);
      typename Setup<math_policy_type>::vector3_type const r = scene.position(2);
      path.push_back(r(0));
      path.push_back(r(1));
      path.push_back(r(2));
    }
    return path;
  }

  size_t const steps = 100;

} // anonymous namespace

BOOST_AUTO_TEST_SUITE(opentissue_dynamics_mbd_math_policy_equivalence);

BOOST_AUTO_TEST_CASE(mass_matrices_default_ublas)   { check_mass_matrices< OpenTissue::mbd::default_ublas_math_policy<double> >(); }
BOOST_AUTO_TEST_CASE(mass_matrices_optimized_ublas) { check_mass_matrices< OpenTissue::mbd::optimized_ublas_math_policy<double> >(); }

//
// Before comparing policies, make sure the reference does something worth comparing: the box
// has to move, and the joint has to hold it at its distance from the anchor.
//
BOOST_AUTO_TEST_CASE(pendulum_swings_and_the_joint_holds)
{
  typedef OpenTissue::mbd::default_ublas_math_policy<double> policy;

  Pendulum<policy> scene;
  Setup<policy>::vector3_type const start = scene.position(2);

  for(size_t s = 0; s < steps; ++s)
    scene.m_simulator.run(0.01);

  Setup<policy>::vector3_type const end   = scene.position(2);
  Setup<policy>::vector3_type const moved = end - start;

  BOOST_CHECK(std::sqrt(moved * moved) > 0.5);

  // The pivot is the joint point, 2 along the fixed anchor's x axis from its centre: (0,7,0).
  // The swinging box's socket is 2 from its own centre, so the ball joint holds that centre
  // on a sphere of radius 2 about the pivot. With the stabilisation term on it should stay
  // there to well within a centimetre.
  Setup<policy>::vector3_type const pivot = scene.position(1) + Setup<policy>::vector3_type(2, 0, 0);
  Setup<policy>::vector3_type const arm   = end - pivot;
  BOOST_CHECK_SMALL(std::sqrt(arm * arm) - 2.0, 1e-2);
}

//
// The equivalence itself. The policies reach the solution by different arithmetic, so the
// trajectories agree to round-off rather than exactly.
//
BOOST_AUTO_TEST_CASE(pendulum_trajectory_is_the_same_on_both_policies)
{
  std::vector<double> const reference = trajectory< OpenTissue::mbd::default_ublas_math_policy<double>   >(steps);
  std::vector<double> const optimized = trajectory< OpenTissue::mbd::optimized_ublas_math_policy<double> >(steps);

  BOOST_REQUIRE_EQUAL(reference.size(), optimized.size());

  double worst = 0.0;
  for(size_t i = 0; i < reference.size(); ++i)
    worst = std::max(worst, std::fabs(reference[i] - optimized[i]));

  BOOST_TEST_MESSAGE("largest position difference between the policies over " << steps << " steps: " << worst);

  BOOST_CHECK_SMALL(worst, 1e-9);
}

BOOST_AUTO_TEST_SUITE_END();

//
// OpenTissue Template Library Demo
// - A specific demonstration of the flexibility of OTTL.
// Copyright (C) 2009 Department of Computer Science, University of Copenhagen.
//
// OTTL and OTTL Demos are licensed under zlib.
//
// A multibody simulation, rendered in ParaView. See documentation/paraview_example_double_pendulum.md.
//
// A double pendulum: two rigid rods, each 1 m long and 1 kg, joined end to end by hinges, the
// upper one to a fixed pivot. Both start horizontal and at rest. OpenTissue's multibody engine
// (dynamics/mbd) moves them: the rods are rigid bodies, the hinges are joint constraints, and
// a constraint solver finds the forces that keep the joints together.
//
// A double pendulum is chaotic: the smallest difference in how it starts, or in how it is
// computed, grows exponentially until the motion is completely different. To show that, the
// program also integrates the pendulum's equations of motion directly, far more accurately,
// and writes that "reference" pendulum next to the engine's. The two move together for about
// seven seconds, then part ways.
//
// Written into the current directory:
//
//   pendulum_0000.vtk ...      the engine's pendulum, one file per frame
//   reference_0000.vtk ...     the reference pendulum, one file per frame
//   trace_0000.vtk ...         the path of the engine pendulum's tip so far
//
// Then, in the same directory:
//
//   pvbatch render.py          renders frames to pendulum_*.png without opening a window
//
#include <OpenTissue/configuration.h>

#include <OpenTissue/dynamics/mbd/math/mbd_optimized_ublas_math_policy.h>
#include <OpenTissue/dynamics/mbd/mbd.h>

#include <cmath>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

template<typename types>
class Stepper
  : public OpenTissue::mbd::DynamicsStepper< types, OpenTissue::mbd::ProjectedGaussSeidel<typename types::math_policy> >
{};

// Collision detection is part of every simulator, but nothing here collides: the arms swing in
// planes a few centimetres apart, as on a real double pendulum, and the engine skips bodies
// that share a joint.
template<typename types>
class Detection
  : public OpenTissue::mbd::CollisionDetection<
      types
    , OpenTissue::mbd::SpatialHashing
    , OpenTissue::mbd::GeometryDispatcher
    , OpenTissue::mbd::SingleGroupAnalysis
    >
{};

typedef OpenTissue::mbd::optimized_ublas_math_policy<double>  math_policy;

typedef OpenTissue::mbd::Types<
    math_policy
  , OpenTissue::mbd::NoSleepyPolicy
  , Stepper
  , Detection
  , OpenTissue::mbd::ExplicitFixedStepSimulator
  >                                                           types;

typedef types::simulator_type                                 simulator_type;
typedef types::body_type                                      body_type;
typedef types::socket_type                                    socket_type;
typedef types::configuration_type                             configuration_type;
typedef types::material_library_type                          material_library_type;
typedef math_policy::vector3_type                             vector3_type;
typedef math_policy::quaternion_type                          quaternion_type;
typedef math_policy::matrix3x3_type                           matrix3x3_type;
typedef math_policy::coordsys_type                            coordsys_type;
typedef OpenTissue::mbd::Gravity<types>                       gravity_type;
typedef OpenTissue::mbd::HingeJoint<types>                    hinge_type;
typedef OpenTissue::geometry::OBB<math_policy>                box_type;

double const pi = 3.14159265358979323846;

// The pendulum. The rods swing in the x-y plane, with y up; the hinges turn about z.
double const length  = 1.0;       // of each rod, m
double const mass    = 1.0;       // of each rod, kg
double const gravity = 9.81;      // m/s^2
double const theta1  = pi / 2;    // starting angles from straight down: both horizontal
double const theta2  = pi / 2;

// Time. The time step decides the engine's accuracy; see the documentation for others.
double       const dt              = 0.0025;
unsigned int const steps           = 4000;   // 10 seconds
unsigned int const steps_per_frame = 16;     // 251 frames, 0.04 s apart
unsigned int const steps_per_line  = 200;    // a line of checks every 0.5 s
unsigned int const steps_per_trace = 4;      // a point on the tip's path every 0.01 s

/**
* The reference: the equations of motion of two identical uniform rods, as Lagrangian mechanics
* gives them (for example, Wikipedia's "Double pendulum"), in the rods' angles from straight
* down and their generalized momenta, integrated with fourth-order Runge-Kutta.
*/
class Reference
{
public:

  double m_theta1, m_theta2, m_p1, m_p2;

  Reference(double theta1_, double theta2_)
    : m_theta1(theta1_), m_theta2(theta2_), m_p1(0.0), m_p2(0.0)
  {}

  void step(double h)
  {
    double k1[4], k2[4], k3[4], k4[4], s[4] = { m_theta1, m_theta2, m_p1, m_p2 }, t[4];
    derivative(s, k1);
    for(int i = 0; i < 4; ++i) t[i] = s[i] + 0.5 * h * k1[i];
    derivative(t, k2);
    for(int i = 0; i < 4; ++i) t[i] = s[i] + 0.5 * h * k2[i];
    derivative(t, k3);
    for(int i = 0; i < 4; ++i) t[i] = s[i] + h * k3[i];
    derivative(t, k4);
    for(int i = 0; i < 4; ++i) s[i] += h / 6.0 * ( k1[i] + 2.0 * k2[i] + 2.0 * k3[i] + k4[i] );
    m_theta1 = s[0]; m_theta2 = s[1]; m_p1 = s[2]; m_p2 = s[3];
  }

protected:

  static void derivative(double const s[4], double d[4])
  {
    double const c  = std::cos( s[0] - s[1] );
    double const sn = std::sin( s[0] - s[1] );
    double const k  = 6.0 / ( mass * length * length );
    double const w1 = k * ( 2.0 * s[2] - 3.0 * c * s[3] ) / ( 16.0 - 9.0 * c * c );
    double const w2 = k * ( 8.0 * s[3] - 3.0 * c * s[2] ) / ( 16.0 - 9.0 * c * c );
    d[0] = w1;
    d[1] = w2;
    d[2] = -0.5 * mass * length * length * (  w1 * w2 * sn + 3.0 * gravity / length * std::sin( s[0] ) );
    d[3] = -0.5 * mass * length * length * ( -w1 * w2 * sn +       gravity / length * std::sin( s[1] ) );
  }
};

/**
* The direction from a rod's upper end to its lower end, for an angle from straight down.
*/
vector3_type direction(double theta)
{
  return vector3_type( std::sin(theta), -std::cos(theta), 0.0 );
}

/**
* The double pendulum in the multibody engine.
*/
class Pendulum
{
public:

  // Declaration order is destruction order reversed, and it matters: the configuration refers
  // to the bodies and the joints and tears them down in its own destructor, so everything it
  // refers to must be declared before it. The joints refer to the sockets, the bodies to their
  // geometry and gravity.
  gravity_type            m_gravity;
  box_type                m_pivot_box;
  box_type                m_rod_box;
  std::vector<body_type>  m_bodies;       // the fixed pivot, then the upper and lower rods
  socket_type             m_sockets[4];
  hinge_type              m_hinges[2];
  material_library_type   m_library;
  configuration_type      m_configuration;
  simulator_type          m_simulator;

  Pendulum()
    : m_bodies(3)
  {
    matrix3x3_type const identity = OpenTissue::math::diag(1.0);
    m_pivot_box.set( vector3_type(0, 0, 0), identity, vector3_type(0.05, 0.05, 0.05) );
    m_rod_box.set(   vector3_type(0, 0, 0), identity, vector3_type(0.5 * length, 0.02, 0.02) );

    // The pivot sits behind the upper rod's plane, and the lower rod in front of it, so that
    // the lower rod passes the pivot freely. The hinges turn about z, so these offsets along z
    // do not change the motion.
    double const behind = -0.05, in_front = 0.05;

    m_bodies[0].set_position( vector3_type(0, 0, behind) );
    m_bodies[0].set_orientation( quaternion_type(1, 0, 0, 0) );
    m_bodies[0].set_geometry( &m_pivot_box );
    m_bodies[0].set_fixed( true );
    m_configuration.add( &m_bodies[0] );

    // Each rod's body frame has the rod along x, so its orientation is a turn about z that
    // takes x onto the rod's direction. Mass properties: a uniform rod, I = m L^2 / 12 about
    // its centre, and a thin one about its own axis.
    vector3_type const centres[2] = {
        0.5 * length * direction(theta1)
      , length * direction(theta1) + 0.5 * length * direction(theta2) + vector3_type(0, 0, in_front)
    };
    double const angles[2] = { theta1, theta2 };
    double const I_across  = mass * length * length / 12.0;
    double const I_along   = mass * 0.04 * 0.04 / 6.0;
    for(int r = 0; r < 2; ++r)
    {
      body_type & rod = m_bodies[r + 1];
      double const turn = angles[r] - 0.5 * pi;
      rod.attach( &m_gravity );
      rod.set_position( centres[r] );
      rod.set_orientation( quaternion_type( std::cos(0.5 * turn), 0, 0, std::sin(0.5 * turn) ) );
      rod.set_geometry( &m_rod_box );
      rod.set_fixed( false );
      rod.set_mass( mass );
      rod.set_inertia_bf( matrix3x3_type( I_along, 0, 0,  0, I_across, 0,  0, 0, I_across ) );
      m_configuration.add( &rod );
    }

    // A hinge turns about the z axis of its sockets' frames, which here is world z.
    quaternion_type const no_turn;
    m_sockets[0].init( m_bodies[0], coordsys_type( vector3_type( 0, 0, -behind ), no_turn ) );
    m_sockets[1].init( m_bodies[1], coordsys_type( vector3_type( -0.5 * length, 0, 0 ), no_turn ) );
    m_sockets[2].init( m_bodies[1], coordsys_type( vector3_type(  0.5 * length, 0, 0.5 * in_front ), no_turn ) );
    m_sockets[3].init( m_bodies[2], coordsys_type( vector3_type( -0.5 * length, 0, -0.5 * in_front ), no_turn ) );
    m_hinges[0].connect( m_sockets[0], m_sockets[1] );
    m_hinges[1].connect( m_sockets[2], m_sockets[3] );
    for(int h = 0; h < 2; ++h)
    {
      m_hinges[h].set_frames_per_second( 1.0 / dt );
      m_hinges[h].set_error_reduction_parameter( 0.8 );
      m_configuration.add( &m_hinges[h] );
    }

    m_gravity.set_acceleration( vector3_type(0, -gravity, 0) );
    m_simulator.init( m_configuration );
    m_configuration.set_material_library( m_library );

    m_simulator.get_stepper()->get_solver()->set_max_iterations( 30 );
    m_simulator.get_stepper()->warm_starting()     = false;
    m_simulator.get_stepper()->use_stabilization() = true;
    m_simulator.get_stepper()->use_friction()      = false;
    m_simulator.get_stepper()->use_bounce()        = false;
  }

  void run() { m_simulator.run( dt ); }

  /**
  * A rod's angle from straight down, read from its orientation.
  */
  double angle(int rod) const
  {
    quaternion_type Q;
    m_bodies[rod + 1].get_orientation( Q );
    vector3_type const x = Q.rotate( vector3_type(1, 0, 0) );
    return std::atan2( x(0), -x(1) );
  }

  /**
  * Kinetic plus potential energy.
  */
  double energy() const
  {
    double E = 0.0;
    for(int r = 1; r <= 2; ++r)
    {
      vector3_type position, velocity, spin;
      m_bodies[r].get_position( position );
      m_bodies[r].get_velocity( velocity );
      m_bodies[r].get_spin( spin );
      E += 0.5 * mass * ( velocity * velocity ) + 0.5 * ( mass * length * length / 12.0 ) * spin(2) * spin(2) + mass * gravity * position(1);
    }
    return E;
  }

  /**
  * How far apart the two halves of each hinge are: the joint error.
  */
  double joint_gap() const
  {
    vector3_type const a = m_sockets[0].get_anchor_world() - m_sockets[1].get_anchor_world();
    vector3_type const b = m_sockets[2].get_anchor_world() - m_sockets[3].get_anchor_world();
    return std::max( std::sqrt( a * a ), std::sqrt( b * b ) );
  }

  /**
  * The pivot, the elbow and the tip, where the rods' ends are.
  */
  void joints(vector3_type points[3]) const
  {
    points[0] = m_sockets[1].get_anchor_world();
    points[1] = m_sockets[3].get_anchor_world();
    quaternion_type Q;
    vector3_type r;
    m_bodies[2].get_orientation( Q );
    m_bodies[2].get_position( r );
    points[2] = r + Q.rotate( vector3_type( 0.5 * length, 0, 0 ) );
  }

private:

  Pendulum(Pendulum const &);
  Pendulum & operator=(Pendulum const &);
};

/**
* Writes a pendulum as legacy VTK polydata: three points -- pivot, elbow, tip -- joined by two
* lines, one per rod.
*/
void write_pendulum(std::string const & name, vector3_type const points[3])
{
  std::ofstream out(name.c_str());
  out << "# vtk DataFile Version 3.0\n"
      << "OpenTissue double pendulum\n"
      << "ASCII\n"
      << "DATASET POLYDATA\n"
      << "POINTS 3 double\n";
  for(int i = 0; i < 3; ++i)
    out << points[i](0) << " " << points[i](1) << " " << points[i](2) << "\n";
  out << "LINES 2 6\n"
      << "2 0 1\n"
      << "2 1 2\n";
}

/**
* Writes the path of the tip so far as one polyline, with the time at each point.
*/
void write_trace(std::string const & name, std::vector<vector3_type> const & path, std::vector<double> const & times)
{
  size_t const n = path.size();
  std::ofstream out(name.c_str());
  out << "# vtk DataFile Version 3.0\n"
      << "OpenTissue double pendulum tip path\n"
      << "ASCII\n"
      << "DATASET POLYDATA\n"
      << "POINTS " << n << " double\n";
  for(size_t i = 0; i < n; ++i)
    out << path[i](0) << " " << path[i](1) << " " << path[i](2) << "\n";
  out << "LINES 1 " << n + 1 << "\n" << n;
  for(size_t i = 0; i < n; ++i)
    out << " " << i;
  out << "\nPOINT_DATA " << n << "\n"
      << "SCALARS time double 1\nLOOKUP_TABLE default\n";
  for(size_t i = 0; i < n; ++i)
    out << times[i] << "\n";
}

std::string frame_name(char const * prefix, unsigned int frame)
{
  std::ostringstream name;
  name << prefix << "_" << std::setw(4) << std::setfill('0') << frame << ".vtk";
  return name.str();
}

/**
* An angle in (-pi, pi].
*/
double wrap(double a)
{
  while(a >  pi) a -= 2.0 * pi;
  while(a <= -pi) a += 2.0 * pi;
  return a;
}

int main()
{
  Pendulum  pendulum;
  Reference reference( theta1, theta2 );

  // The reference takes 100 Runge-Kutta steps for each step of the engine, which makes it
  // exact to far more digits than matter here.
  unsigned int const reference_substeps = 100;

  // The tip's path, as it is written for ParaView: a point every 0.01 s, finer than the frames.
  std::vector<vector3_type> path;
  std::vector<double>       times;

  // Checks, printed as it runs:
  //  - the engine's angles against the reference's: they agree until chaos takes over;
  //  - the total energy should stay constant, since nothing here loses energy;
  //  - the joint gap, how far apart the two halves of a hinge have drifted.
  double const E0 = pendulum.energy();
  std::printf("time step %.4f s; angles from straight down, in radians\n", dt);
  std::printf("  time   upper rod: engine  reference   lower rod: engine  reference   energy change   joint gap\n");

  for(unsigned int step = 0; step <= steps; ++step)
  {
    double const t = step * dt;

    if(step % steps_per_trace == 0)
    {
      vector3_type points[3];
      pendulum.joints( points );
      path.push_back( points[2] );
      times.push_back( t );
    }

    if(step % steps_per_frame == 0)
    {
      unsigned int const frame = step / steps_per_frame;

      vector3_type points[3];
      pendulum.joints( points );
      write_pendulum( frame_name("pendulum", frame), points );
      write_trace( frame_name("trace", frame), path, times );

      // The reference pendulum, drawn in the planes of the engine's rods.
      vector3_type ref[3];
      ref[0] = points[0];
      ref[1] = ref[0] + length * direction( reference.m_theta1 );
      ref[2] = ref[1] + length * direction( reference.m_theta2 );
      write_pendulum( frame_name("reference", frame), ref );
    }

    if(step % steps_per_line == 0)
    {
      std::printf("  %4.1f       %7.3f    %7.3f            %7.3f    %7.3f        %+7.3f J     %.1e m\n"
        , t
        , wrap( pendulum.angle(0) ), wrap( reference.m_theta1 )
        , wrap( pendulum.angle(1) ), wrap( reference.m_theta2 )
        , pendulum.energy() - E0
        , pendulum.joint_gap() );
    }

    if(step < steps)
    {
      pendulum.run();
      for(unsigned int k = 0; k < reference_substeps; ++k)
        reference.step( dt / reference_substeps );
    }
  }

  std::printf("wrote pendulum_, reference_ and trace_0000.vtk .. %04u.vtk\n", steps / steps_per_frame);
  return 0;
}

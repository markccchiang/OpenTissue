//
// OpenTissue Template Library Demo
// - A specific demonstration of the flexibility of OTTL.
// Copyright (C) 2008 Department of Computer Science, University of Copenhagen.
//
// OTTL and OTTL Demos are licensed under zlib.
//
// This is the minimal demo, and the starting point for porting the rest of the demos that
// commit dc6f0aa removed. Compared with the old GLUT version:
//
//   * derive from OpenTissue::graphics::GlfwApplication instead of PerspectiveViewApplication
//   * do_init()    -> init()
//   * do_display() -> update(double), which is called once per frame
//   * do_action()  -> action(unsigned char)
//   * do_get_title() and the window size go to the base constructor
//   * init_glut_application() -> createApplication(), with OT_INJECT_MAIN in CMakeLists.txt
//
// The camera, lighting, projection and mouse navigation all come from the base class, as
// they did before.
//
#include <OpenTissue/configuration.h>

#include <OpenTissue/core/math/math_basic_types.h>
#include <OpenTissue/graphics/glfw/glfw_application.h>
#include <OpenTissue/graphics/core/gl/gl_draw_frame.h>

class Application : public OpenTissue::graphics::GlfwApplication
{
public:

  typedef OpenTissue::math::default_math_types  math_types;

  Application()
    : OpenTissue::graphics::GlfwApplication("My Minimal Application")
  {}

public:

  void init() override
  {}

  void update(double /*timestep*/) override
  {
    OpenTissue::gl::DrawFrame( math_types::vector3_type(), math_types::quaternion_type() );
  }

  void action(unsigned char /*choice*/) override
  {}

};

OpenTissue::graphics::GlfwApplication::Ptr createApplication(int /*argc*/, char ** /*argv*/)
{
  return OpenTissue::graphics::GlfwApplication::New<Application>();
}

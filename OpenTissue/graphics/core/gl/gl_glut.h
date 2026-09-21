#ifndef OPENTISSUE_GRAPHICS_CORE_GL_GL_GLUT_H
#define OPENTISSUE_GRAPHICS_CORE_GL_GL_GLUT_H
//
// OpenTissue Template Library
// - A generic toolbox for physics-based modeling and simulation.
// Copyright (C) 2008 Department of Computer Science, University of Copenhagen.
//
// OTTL is licensed under zlib: http://opensource.org/licenses/zlib-license.php
//
#include <OpenTissue/configuration.h>

//
// GLUT, in the one place that knows where it lives on each platform.
//
// This used to sit inside gl.h, which meant every single piece of OpenTissue that touched
// OpenGL needed GLUT to be installed, even though only the two text helpers -- gl_draw_string.h
// and gl_on_screen_display.h -- actually call into it. Those two include this header directly
// now, so everything else only needs OpenGL and GLEW.
//
// The application backend no longer uses GLUT at all; that is GLFW's job. Replacing the
// remaining text rendering is what would finally remove the dependency.
//
#include <OpenTissue/graphics/core/gl/gl.h>

#if defined(__APPLE__) && !defined(VMDMESA)
#  include <GLUT/glut.h>
#else
#  include <GL/freeglut.h>
#endif

//OPENTISSUE_GRAPHICS_CORE_GL_GL_GLUT_H
#endif

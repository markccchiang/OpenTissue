#ifndef OPENTISSUE_UTILITY_GL_GL_GL_H
#define OPENTISSUE_UTILITY_GL_GL_GL_H
//
// OpenTissue Template Library
// - A generic toolbox for physics-based modeling and simulation.
// Copyright (C) 2008 Department of Computer Science, University of Copenhagen.
//
// OTTL is licensed under zlib: http://opensource.org/licenses/zlib-license.php
//
#include <OpenTissue/configuration.h>

#ifdef WIN32
#  define WIN32_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#  undef WIN32_LEAN_AND_MEAN
#  undef NOMINMAX
#endif
#include <GL/glew.h>
#if defined(__APPLE__) && !defined (VMDMESA)
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

// GLUT is deliberately not included here. Only the text helpers need it, and they include
// <OpenTissue/graphics/core/gl/gl_glut.h> themselves, so the rest of the GL code builds
// against OpenGL and GLEW alone.

//OPENTISSUE_UTILITY_GL_GL_GL_H
#endif

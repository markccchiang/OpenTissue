#-----------------------------------------------------------------------------
#
# Create OpenTissue exportable target
#

if(NOT TARGET headers)
  add_library(headers INTERFACE)
endif()

if(NOT TARGET depends)
  add_library(depends INTERFACE)
endif()

if(NOT TARGET OpenTissue)
  add_library(OpenTissue INTERFACE)
endif()

# Boost::headers carries the Boost include directories. Without it, anything that consumes
# OpenTissue::headers from outside this build tree fails on the first OpenTissue header that
# includes Boost (math_random.h includes <boost/cast.hpp>, for instance). In-tree targets
# happened to get the include path from Boost::unit_test_framework, which hid the problem.
target_link_libraries(headers
  INTERFACE
    Boost::headers
    Boost::disable_autolinking
)

target_include_directories(headers
  INTERFACE
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
)

# MSVC's default object file format allows 65,279 sections, and a translation unit that
# composes an OpenTissue simulator goes past that: the template instantiations each need
# their own COMDAT section. The result is
#
#   error C1128: number of sections exceeded object file format limit: compile with /bigobj
#
# /bigobj raises the limit. It changes nothing but the object file format, so it is safe to
# apply everywhere rather than hunting for the translation units that happen to cross the
# line -- and consumers instantiating the same templates need it just as much as this
# project's own tests do, which is why it goes on the interface target.
#
# This only surfaced once unit_multibody_build_test compiled on Windows for the first time.
# Until Qhull was found there, the whole multibody test directory was skipped.
target_compile_options(headers
  INTERFACE
    $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
)

target_compile_definitions(headers
  INTERFACE
    $<$<CXX_COMPILER_ID:MSVC>:_SCL_SECURE_NO_WARNINGS>
    $<$<CXX_COMPILER_ID:MSVC>:_SCL_SECURE_NO_DEPRECATE>
    $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
    $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_DEPRECATE>
    $<$<CXX_COMPILER_ID:MSVC>:_SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS>
    $<$<CXX_COMPILER_ID:GNU>:_USE_MATH_DEFINES>
    # Apple deprecated the whole of OpenGL in 10.14, and OpenTissue's GL helpers use the
    # fixed-function API throughout, so a build produces hundreds of these warnings. This
    # was guarded on CXX_COMPILER_ID:GNU, which never matches on macOS -- the compiler
    # there is AppleClang -- so the define never actually took effect where it is needed.
    $<$<PLATFORM_ID:Darwin>:GL_SILENCE_DEPRECATION>
)

target_link_libraries(OpenTissue
  INTERFACE
    headers
)

#-------------------------------------------------------------------------------
#
# Generate configuration.h file
# This way platform and system specific settings can be added to the
# configuration.h file
#
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/configuration.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/configuration.h"
  @ONLY
)

# OpenTissueGraphics needs the OpenGL/GLEW/GLFW stack. Skip it when that is unavailable so
# that the default build target still works on a machine without a graphics stack.
if(OPENTISSUE_HAVE_GRAPHICS)
  add_subdirectory(graphics)
else()
  message(STATUS "Skipping OpenTissueGraphics (needs OpenGL, GLEW and GLFW)")
endif()

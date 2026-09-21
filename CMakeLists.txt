cmake_minimum_required(VERSION 3.25)
project(OpenTissue VERSION 0.994
                   DESCRIPTION "A header only c++ library collection of generic algorithms and data structures for rapid development of interactive modeling and simulation."
                   HOMEPAGE_URL "https://github.com/erleben/OpenTissue")

# CMake 4 removed the bundled FindBoost module in favour of the BoostConfig.cmake package that
# Boost itself ships. Opt in explicitly so the removal is not merely a deprecation warning.
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Use -std=c++17 rather than -std=gnu++17, so the standard setting above actually
# constrains what compiles and the code stays portable across compilers.
set(CMAKE_CXX_EXTENSIONS OFF)

#-----------------------------------------------------------------------------
#
# Avoid in-source builds.
#
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(
    FATAL_ERROR
    "CMake generation for OpenTissue is not allowed within the source directory!"
    )
endif()

list(INSERT CMAKE_MODULE_PATH 0 ${PROJECT_SOURCE_DIR}/cmake)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

#-----------------------------------------------------------------------------
#
# Global variables that control the behaviour of CMake. Use these in
# the GUI to turn on/off different kind of things.
#
option(OPENTISSUE_ENABLE_UNIT_TESTS "Build unit test" OFF)
option(OPENTISSUE_ENABLE_DOCUMENTATION "Build documentation" OFF)
option(OPENTISSUE_ENABLE_DEMOS "Build demos" OFF)

#-----------------------------------------------------------------------------
#
# Try to find Boost (http://www.boost.org/), Boost is needed by almost all
# OpenTissue code, one have to make sure this one works!
# FindBoost
set(OPENTISSUE_BOOST_VERSION 1.39.0)
if(OPENTISSUE_ENABLE_UNIT_TESTS)
  # Only ask for the compiled component when we actually build the tests; passing an empty
  # COMPONENTS list is an error in config mode.
  find_package(Boost ${OPENTISSUE_BOOST_VERSION} REQUIRED COMPONENTS unit_test_framework)
else()
  find_package(Boost ${OPENTISSUE_BOOST_VERSION} REQUIRED)
endif()

if(OPENTISSUE_ENABLE_DEMOS)
  #-----------------------------------------------------------------------------
  #
  # Try to find OpenGL (http://www.opengl.org/)
  #
  # OpenGL is used by the default demo applications in OpenTissue. The
  # visualization part of OpenTissue has been designed as separate code
  # pieces. These code pieces are intended for illustration purpose and
  # for brute-force debug utilities for the OpenTissue developers. The
  # openGL code pieces have never been intended to be used by end-users.
  # Therefore end-users should be able to use OpenTissue without OpenGL.
  #
  # (The GPGPU code pieces that were hard-wired to Cg have been removed; Cg was
  # discontinued in 2012.)
  #
  set(OpenGL_GL_PREFERENCE GLVND)
  find_package(OpenGL REQUIRED)

  #-----------------------------------------------------------------------------
  #
  # GLUT. The GLUT application backend is gone -- GLFW replaced it and nothing
  # referenced the GLUT one -- but the GL text helpers (gl_draw_string.h and
  # gl_on_screen_display.h) still call glutStrokeCharacter and friends, and
  # graphics/core/gl/gl.h includes the GLUT header unconditionally. So GLUT is
  # still required to compile anything that draws text. Removing that is part of
  # finishing the move to GLFW.
  #
  find_package(GLUT REQUIRED)

  # Let CMake locate glfw3 through the usual search paths (CMAKE_PREFIX_PATH, Homebrew,
  # vcpkg, ...). Hard-coding /usr/local broke on Apple Silicon, where Homebrew installs
  # into /opt/homebrew. Point glfw3_ROOT at a custom prefix if you need one.
  find_package(glfw3 REQUIRED)

  #-----------------------------------------------------------------------------
  #
  # Try to get GLEW (http://glew.sourceforge.net/). Our demo application
  # framework relies on GLEW for initialization of any OpenGL extentions.
  # End-users are not required to use GLEW.
  #
  # As long as if they use any GPGPU stuff from OpenTissue they provide their
  # own extention initialization. If no GPGPU stuff is used then this library
  # dependency can be ignored.
  #
  find_package(GLEW REQUIRED)

endif()

#-----------------------------------------------------------------------------
#
# Resolve the optional third-party libraries (TinyXML, Qhull, TetGen, Triangle,
# libpng). None of these is required to configure or to use the bulk of
# OpenTissue -- see cmake/OpenTissueDependencies.cmake for what each one gates
# and for the licensing caveats on TetGen and Triangle.
#
include(OpenTissueDependencies)

#-----------------------------------------------------------------------------
#
# Look into subfolders
#
add_subdirectory(OpenTissue)

if(OPENTISSUE_ENABLE_DEMOS)
  add_subdirectory(demos)
endif()

if(OPENTISSUE_ENABLE_DOCUMENTATION)
  add_subdirectory(documentation)
endif()

if(OPENTISSUE_ENABLE_UNIT_TESTS)
  option(BUILD_TESTING "Enable cmake testing utils." ON)
  include(CTest)
  add_subdirectory(unit_tests)
endif()

#-----------------------------------------------------------------------------
#
# Option for removing deprecated code from deprecated preprocessor
#
option(OPENTISSUE_EXCLUDE_DEPRECATED "Exclude deprecated parts of the library" OFF)
if(OPENTISSUE_EXCLUDE_DEPRECATED)
  set(NO_BUILD_DEPRECATED DEFINE_NO_DEPRECATED)
endif()

#-------------------------------------------------------------------------------
#
# Add support for other CMake applications, so it is easier to use OpenTissue.
#
# Recall, that CMake works with tree terms: source-, build- (ie. binary-) and
# isntall- configuration/tree.
#
# OpenTissue is a header only library (mostly), so the build-tree only makes sense for
# demo applications and units. Source- and install trees are basically just all
# the header-files, third-party libraries and cmake finding scripts.
#
include(CMakePackageConfigHelpers)
include(CPack)
include(GNUInstallDirs)

export(PACKAGE OpenTissue) # Makes OT discoverable, via ~/.cmake
export(TARGETS headers depends NAMESPACE OpenTissue:: FILE OpenTissueTargets.cmake)

configure_file(
  "${PROJECT_SOURCE_DIR}/cmake/OpenTissueConfig.cmake.in"
  "${PROJECT_SOURCE_DIR}/OpenTissueConfig.cmake"
  @ONLY
)

configure_package_config_file(
  "${PROJECT_SOURCE_DIR}/cmake/OpenTissueConfig.cmake.in"
  "${PROJECT_BINARY_DIR}/OpenTissueConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenTissue
  PATH_VARS CMAKE_INSTALL_LIBDIR CMAKE_INSTALL_INCLUDEDIR
)

write_basic_package_version_file(
  OpenTissueConfigVersion.cmake
  VERSION ${OpenTissue_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(TARGETS headers depends
  EXPORT OpenTissueTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(EXPORT OpenTissueTargets
  FILE OpenTissueTargets.cmake
  NAMESPACE OpenTissue::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenTissue
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/OpenTissueConfigVersion.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/OpenTissueConfig.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenTissue
)

install(DIRECTORY
  ${CMAKE_CURRENT_SOURCE_DIR}/OpenTissue
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  PATTERN "*.h.in" EXCLUDE
  PATTERN "*.cmake" EXCLUDE
  PATTERN "CMakeLists.txt" EXCLUDE
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/OpenTissue/configuration.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/OpenTissue
)

#-----------------------------------------------------------------------------
#
# Summarise which optional dependencies were resolved.
#
ot_report_dependencies()

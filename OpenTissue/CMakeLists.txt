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

target_compile_definitions(headers
  INTERFACE
    $<$<CXX_COMPILER_ID:MSVC>:_SCL_SECURE_NO_WARNINGS>
    $<$<CXX_COMPILER_ID:MSVC>:_SCL_SECURE_NO_DEPRECATE>
    $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
    $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_DEPRECATE>
    $<$<CXX_COMPILER_ID:MSVC>:_SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS>
    $<$<CXX_COMPILER_ID:GNU>:_USE_MATH_DEFINES>
    $<$<CXX_COMPILER_ID:GNU>:GL_SILENCE_DEPRECATION>
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

add_subdirectory(graphics)

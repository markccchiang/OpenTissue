#-----------------------------------------------------------------------------
#
# Header self-containment check.
#
# Every OpenTissue header should compile on its own. A header that only works
# because some *other* header happened to be included first is a latent build
# break: it fails the moment a user includes it directly, or the moment a
# standard library stops leaking a transitive include. That is exactly how this
# library ended up with ~200 headers that no longer compiled -- libc++ stopped
# pulling in <iostream>, <vector> and friends behind the scenes.
#
# This target generates one translation unit per header, each containing nothing
# but the #include, and compiles them all. Headers that are not self-contained
# yet are listed in known_not_self_contained.txt.
#
# It is off by default because it adds ~900 translation units to the build.
# Turn it on in CI:
#
#   cmake -S . -B build -DOPENTISSUE_ENABLE_HEADER_TESTS=ON
#   cmake --build build --target opentissue_header_check
#
#-----------------------------------------------------------------------------

set(_exclusion_file "${CMAKE_CURRENT_SOURCE_DIR}/known_not_self_contained.txt")

file(STRINGS "${_exclusion_file}" _raw_exclusions)
set(_excluded "")
foreach(_line IN LISTS _raw_exclusions)
  string(STRIP "${_line}" _line)
  if(_line AND NOT _line MATCHES "^#")
    list(APPEND _excluded "${_line}")
  endif()
endforeach()
list(LENGTH _excluded _excluded_count)

file(GLOB_RECURSE _headers
  RELATIVE "${PROJECT_SOURCE_DIR}"
  CONFIGURE_DEPENDS
  "${PROJECT_SOURCE_DIR}/OpenTissue/*.h"
)

set(_generated "")
set(_skipped 0)
foreach(_header IN LISTS _headers)
  if(_header IN_LIST _excluded)
    math(EXPR _skipped "${_skipped} + 1")
    continue()
  endif()

  # One TU per header, named after the header so a failure is easy to trace.
  string(REGEX REPLACE "[/.]" "_" _stem "${_header}")
  set(_tu "${CMAKE_CURRENT_BINARY_DIR}/tu/${_stem}.cpp")
  if(NOT EXISTS "${_tu}")
    file(WRITE "${_tu}" "#include <${_header}>\n")
  endif()
  list(APPEND _generated "${_tu}")
endforeach()

list(LENGTH _generated _checked_count)

# An object library is enough: we only care that each TU compiles, and this way
# nothing has to link.
add_library(opentissue_header_check OBJECT ${_generated})

target_link_libraries(opentissue_header_check PRIVATE OpenTissue)

# Deliberately no -Wall here. A header that is not self-contained fails this check as a
# hard compile error already, since C++ has no implicit declarations, so extra warnings add
# nothing to what the check is for. They do add a great deal of noise: OpenTissue's code
# standard asks for a block of typedefs at the top of every function, which makes
# -Wunused-local-typedef alone produce thousands of hits and bury real output.

# Optional dependencies are resolved per-header by the exclusion list rather
# than per-target, so link whatever happens to be available.
foreach(_dep TinyXML Qhull::libqhull TetGen Triangle PNG::PNG)
  if(TARGET ${_dep})
    target_link_libraries(opentissue_header_check PRIVATE ${_dep})
  endif()
endforeach()

add_test(NAME header_self_containment
  COMMAND ${CMAKE_COMMAND} --build "${PROJECT_BINARY_DIR}"
                           --config $<CONFIG>
                           --target opentissue_header_check
)

message(STATUS
  "Header self-containment check: ${_checked_count} header(s) checked, "
  "${_skipped} skipped via known_not_self_contained.txt")

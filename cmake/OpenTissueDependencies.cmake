##################################################################################################
#
# Third-party dependency resolution for OpenTissue.
#
# OpenTissue itself is header-only and, for the most part, self-contained. Only a handful of
# headers reach out to a third-party library, and each of those is optional: if the library is
# not available, the corresponding headers simply cannot be included, but everything else in
# OpenTissue still configures and builds.
#
# This file therefore never fails the configure step because a dependency is missing. It
# discovers what is available, reports it, and lets the caller decide what to do.
#
# Resolution order for each dependency:
#
#   1. An already-defined target (e.g. injected by a superproject).
#   2. find_package(), which also picks up system/Homebrew/vcpkg installs.
#   3. For TinyXML only, a pinned source download (see the note on fetching below).
#
# Fetching
# ^^^^^^^^
#
# Only TinyXML is fetched automatically. It is dead upstream (last release 2011), is not
# packaged by most distributions, and is zlib-licensed like OpenTissue itself, so vendoring it
# at configure time is both safe and the only practical option. Everything else is widely
# packaged and is expected to come from the system. Set OPENTISSUE_FETCH_DEPENDENCIES=OFF to
# forbid all downloads.
#
# Licensing
# ^^^^^^^^^
#
# OpenTissue is zlib-licensed and advertises itself as free for commercial use. Two of the
# optional dependencies are not compatible with that promise and are therefore OFF by default
# and never downloaded automatically -- enabling them is an explicit, informed choice:
#
#   * Triangle (J.R. Shewchuk) is not open source. It is free for non-commercial use only and
#     may not be sold for profit.
#   * TetGen 1.5+ is AGPL-3.0, which is strongly copyleft.
#
##################################################################################################

include_guard(GLOBAL)
include(FetchContent)

option(OPENTISSUE_FETCH_DEPENDENCIES
  "Allow OpenTissue to download missing permissively-licensed dependencies at configure time" ON)

option(OPENTISSUE_WITH_TINYXML
  "Enable the XML readers/writers under kinematics/*/io (requires TinyXML, zlib licensed)" ON)
option(OPENTISSUE_WITH_QHULL
  "Enable convex hull and Delaunay utilities (requires Qhull)" ON)
option(OPENTISSUE_WITH_TETGEN
  "Enable t4mesh_tetgen_mesh_lofter.h (requires TetGen -- AGPL-3.0, see note above)" OFF)
option(OPENTISSUE_WITH_TRIANGLE
  "Enable polymesh_compute_delaunay2D.h (requires Triangle -- non-commercial only, see note above)" OFF)

# Records one human-readable line for the configure summary printed by
# ot_report_dependencies(). A global property is used rather than a cache variable because
# cache entries cannot hold multi-line values.
set_property(GLOBAL PROPERTY OT_DEPENDENCY_REPORT "")

function(_ot_report name state detail)
  string(LENGTH "${name}" _len)
  math(EXPR _pad "12 - ${_len}")
  set(_spaces "")
  if(_pad GREATER 0)
    string(REPEAT " " ${_pad} _spaces)
  endif()
  set(_line "${name}${_spaces} ${state}")
  if(detail)
    string(APPEND _line " (${detail})")
  endif()
  # Each line is stored as one list element, so any embedded semicolon has to be escaped or
  # it would split the line in two.
  string(REPLACE ";" "\\;" _line "${_line}")
  set_property(GLOBAL APPEND PROPERTY OT_DEPENDENCY_REPORT "${_line}")
endfunction()

#-------------------------------------------------------------------------------------------------
#
# TinyXML -- used by 14 io/ headers and by the character_animation demo.
#
#-------------------------------------------------------------------------------------------------
set(OPENTISSUE_HAVE_TINYXML OFF)

if(OPENTISSUE_WITH_TINYXML)
  if(NOT TARGET TinyXML)
    find_package(TinyXML QUIET)
  endif()

  if(TARGET TinyXML)
    set(OPENTISSUE_HAVE_TINYXML ON)
    _ot_report("TinyXML" "found" "system")
  elseif(OPENTISSUE_FETCH_DEPENDENCIES)
    # TinyXML 2.6.2 ships no build system at all, so we declare the sources and build the
    # library ourselves below.
    FetchContent_Declare(tinyxml
      URL      "https://downloads.sourceforge.net/project/tinyxml/tinyxml/2.6.2/tinyxml_2_6_2.tar.gz"
      URL_HASH SHA256=15bdfdcec58a7da30adc87ac2b078e4417dbe5392f3afb719f9ba6d062645593
    )
    FetchContent_MakeAvailable(tinyxml)

    # OpenTissue includes it as <TinyXML/tinyxml.h>, but the tarball unpacks into a lowercase
    # "tinyxml" directory. Stage the public headers under a correctly-cased directory so the
    # include works on case-sensitive filesystems too.
    set(_ot_tinyxml_incdir "${CMAKE_CURRENT_BINARY_DIR}/_deps/tinyxml-include")
    file(COPY "${tinyxml_SOURCE_DIR}/tinyxml.h" "${tinyxml_SOURCE_DIR}/tinystr.h"
         DESTINATION "${_ot_tinyxml_incdir}/TinyXML")

    # tinystr.cpp implements TiXmlString, which is only used when TIXML_USE_STL is *not*
    # defined. We always define it, so including that file would just produce an empty object
    # file and a "has no symbols" warning from ranlib.
    add_library(TinyXML STATIC
      "${tinyxml_SOURCE_DIR}/tinyxml.cpp"
      "${tinyxml_SOURCE_DIR}/tinyxmlerror.cpp"
      "${tinyxml_SOURCE_DIR}/tinyxmlparser.cpp"
    )
    target_include_directories(TinyXML PUBLIC "${_ot_tinyxml_incdir}")
    # OpenTissue passes std::string to the TinyXML API, which requires the STL build.
    target_compile_definitions(TinyXML PUBLIC TIXML_USE_STL)
    set_target_properties(TinyXML PROPERTIES POSITION_INDEPENDENT_CODE ON)

    set(OPENTISSUE_HAVE_TINYXML ON)
    _ot_report("TinyXML" "fetched" "2.6.2")
  else()
    _ot_report("TinyXML" "MISSING" "fetching disabled; io/ XML headers unavailable")
  endif()
else()
  _ot_report("TinyXML" "disabled" "OPENTISSUE_WITH_TINYXML=OFF")
endif()

#-------------------------------------------------------------------------------------------------
#
# Qhull -- used by utility_qhull.h (convex hull, Delaunay tetrahedralization).
#
# Not fetched: Qhull is packaged essentially everywhere (brew install qhull,
# apt install libqhull-dev, vcpkg install qhull) and its CMake target names have changed
# across releases, so auto-fetching would be more fragile than useful.
#
#-------------------------------------------------------------------------------------------------
set(OPENTISSUE_HAVE_QHULL OFF)

if(OPENTISSUE_WITH_QHULL)
  if(NOT TARGET Qhull::libqhull)
    find_package(Qhull QUIET)
  endif()

  if(TARGET Qhull::libqhull)
    set(OPENTISSUE_HAVE_QHULL ON)
    _ot_report("Qhull" "found" "system")
  else()
    _ot_report("Qhull" "MISSING" "install qhull; convex hull utilities unavailable")
  endif()
else()
  _ot_report("Qhull" "disabled" "OPENTISSUE_WITH_QHULL=OFF")
endif()

#-------------------------------------------------------------------------------------------------
#
# TetGen -- used by exactly one header, t4mesh_tetgen_mesh_lofter.h.
#
# AGPL-3.0. Off by default and never fetched. Note that the TetGen *file format* readers
# (t4mesh_tetgen_read.h and friends) parse plain text and do NOT need this library.
#
#-------------------------------------------------------------------------------------------------
set(OPENTISSUE_HAVE_TETGEN OFF)

if(OPENTISSUE_WITH_TETGEN)
  if(NOT TARGET TetGen)
    find_package(TetGen QUIET)
  endif()

  if(TARGET TetGen)
    set(OPENTISSUE_HAVE_TETGEN ON)
    _ot_report("TetGen" "found" "AGPL-3.0 -- enabled by request")
  else()
    _ot_report("TetGen" "MISSING" "OPENTISSUE_WITH_TETGEN=ON but TetGen was not found")
  endif()
else()
  _ot_report("TetGen" "off" "AGPL-3.0; opt in with OPENTISSUE_WITH_TETGEN=ON")
endif()

#-------------------------------------------------------------------------------------------------
#
# Triangle -- used by exactly one header, polymesh_compute_delaunay2D.h.
#
# Non-commercial license. Off by default and never fetched.
#
#-------------------------------------------------------------------------------------------------
set(OPENTISSUE_HAVE_TRIANGLE OFF)

if(OPENTISSUE_WITH_TRIANGLE)
  if(NOT TARGET Triangle)
    find_package(Triangle QUIET)
  endif()

  if(TARGET Triangle)
    set(OPENTISSUE_HAVE_TRIANGLE ON)
    _ot_report("Triangle" "found" "non-commercial -- enabled by request")
  else()
    _ot_report("Triangle" "MISSING" "OPENTISSUE_WITH_TRIANGLE=ON but Triangle was not found")
  endif()
else()
  _ot_report("Triangle" "off" "non-commercial; opt in with OPENTISSUE_WITH_TRIANGLE=ON")
endif()

#-------------------------------------------------------------------------------------------------
#
# libpng -- used by gpu/image/io/image_read.h and image_write.h, by the character_animation
# demo and by the unit_png test. Permissively licensed and packaged everywhere, but still
# optional so that a bare checkout configures on a machine without it.
#
#-------------------------------------------------------------------------------------------------
set(OPENTISSUE_HAVE_PNG OFF)

if(NOT TARGET PNG::PNG)
  find_package(PNG QUIET)
endif()

if(TARGET PNG::PNG)
  set(OPENTISSUE_HAVE_PNG ON)
  _ot_report("libpng" "found" "${PNG_VERSION_STRING}")
else()
  _ot_report("libpng" "MISSING" "image io headers unavailable")
endif()

#-------------------------------------------------------------------------------------------------
#
# Print what we ended up with. Called from the top-level CMakeLists.txt once everything else
# has been resolved.
#
#-------------------------------------------------------------------------------------------------
function(ot_report_dependencies)
  get_property(_report GLOBAL PROPERTY OT_DEPENDENCY_REPORT)
  message(STATUS "")
  message(STATUS "OpenTissue optional dependencies:")
  foreach(_line IN LISTS _report)
    message(STATUS "    ${_line}")
  endforeach()
  message(STATUS "")
endfunction()

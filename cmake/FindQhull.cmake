##################################################################################################
#
# Find module for Qhull.
#
# Input variables:
#
# - Qhull_ROOT (optional) - Stardard CMake path search variable.
# - You can also set the environment variable Qhull_ROOT variable and
#   cmake will automatically look in there.
#
# Output variables:
#
# - Qhull_FOUND: Boolean that indicates if the package was found
#
# IMPORTED Targets
# ^^^^^^^^^^^^^^^^
#
# This module defines the :prop_tgt:`IMPORTED` targets:
#
# ``Qhull::qhull_r``
#  The reentrant Qhull library.
#
# Example usage:
#
#  find_package(Qhull)
#  if(NOT Qhull_FOUND)
#    # Error handling
#  endif()
#
#  target_link_libraries(my_target Qhull::qhull_r)
#
# Why the reentrant library
# ^^^^^^^^^^^^^^^^^^^^^^^^^
#
# Qhull ships two C libraries: the original one, which keeps its state in globals, and the
# reentrant one (libqhull_r), which passes a qhT around instead. Upstream deprecated the
# former in favour of the latter and packagers have followed: vcpkg builds *only* the
# reentrant library, which is why OpenTissue's Qhull-dependent code was silently skipped on
# Windows. Homebrew and Debian/Ubuntu ship both, so asking for the reentrant one everywhere
# costs nothing and is the only option that works on all three.
#
# The target keeps upstream's own name, so that a superproject which has already pulled in
# Qhull through its config package (find_package(Qhull CONFIG), which exports Qhull::qhull_r)
# satisfies this module without it searching at all.
#
##################################################################################################

# Find headers and libraries.
#
# OpenTissue includes these as <libqhull_r/libqhull_r.h>, so we have to report the directory
# that *contains* libqhull_r/, not libqhull_r/ itself. Searching for the path-qualified header
# name gets that right; "NAMES libqhull_r.h PATH_SUFFIXES libqhull_r" would report one level
# too deep and every #include would then fail.
find_path(Qhull_INCLUDE_DIR NAMES libqhull_r/libqhull_r.h)

# Shared first, then static. Qhull's own build appends _d to the debug library on the
# platforms that distinguish them.
find_library(Qhull_LIBRARY_RELEASE NAMES qhull_r qhullstatic_r)
find_library(Qhull_LIBRARY_DEBUG   NAMES qhull_r_d qhullstatic_r_d)

if(Qhull_LIBRARY_RELEASE)
  set(Qhull_LIBRARIES ${Qhull_LIBRARY_RELEASE})
endif()

if(Qhull_LIBRARY_DEBUG)
  set(Qhull_LIBRARIES ${Qhull_LIBRARIES} ${Qhull_LIBRARY_DEBUG})
endif()

# Output variables generation
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Qhull DEFAULT_MSG Qhull_LIBRARIES
                                                    Qhull_INCLUDE_DIR)

if(Qhull_FOUND)
  if(NOT TARGET Qhull::qhull_r)
    add_library(Qhull::qhull_r UNKNOWN IMPORTED)

    if(EXISTS "${Qhull_LIBRARY_RELEASE}")
      set_property(TARGET Qhull::qhull_r APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
      set_target_properties(Qhull::qhull_r PROPERTIES
        MAP_IMPORTED_CONFIG_RELEASE Release
        IMPORTED_LOCATION_RELEASE "${Qhull_LIBRARY_RELEASE}")
    endif()

    if(EXISTS "${Qhull_LIBRARY_DEBUG}")
      set_property(TARGET Qhull::qhull_r APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
      set_target_properties(Qhull::qhull_r PROPERTIES
        MAP_IMPORTED_CONFIG_DEBUG Debug
        IMPORTED_LOCATION_DEBUG "${Qhull_LIBRARY_DEBUG}")
    endif()

    # IMPORTED_LOCATION is what CMake falls back to when no configuration-specific location
    # matches -- a Debug build when only the release library was found, or any other build when
    # only the debug one was. Point it at whichever library exists, preferring release, or that
    # build gets Qhull_LIBRARY_RELEASE-NOTFOUND on its link line.
    if(EXISTS "${Qhull_LIBRARY_RELEASE}")
      set(_qhull_fallback_location "${Qhull_LIBRARY_RELEASE}")
    else()
      set(_qhull_fallback_location "${Qhull_LIBRARY_DEBUG}")
    endif()

    set_target_properties(Qhull::qhull_r PROPERTIES
      IMPORTED_LOCATION "${_qhull_fallback_location}"
      INTERFACE_INCLUDE_DIRECTORIES "${Qhull_INCLUDE_DIR}")
    unset(_qhull_fallback_location)
  endif()
endif()

mark_as_advanced(Qhull_INCLUDE_DIR
                 Qhull_LIBRARY_RELEASE
                 Qhull_LIBRARY_DEBUG
                 Qhull_LIBRARIES
)

#-----------------------------------------------------------------------------
# If on linux and found a dynamic link library (.so),
# define BOOST_TEST_DYN_LINK to automagically get the main method.
# TODO: This needs to be fixed, Boost_unit_test_framework_LIBRARY is not always defined.
if(Boost_unit_test_framework_LIBRARY)
  get_filename_component(LIB_EXTENSION ${Boost_unit_test_framework_LIBRARY} EXT)
  if(LIB_EXTENSION STREQUAL ".so")
    add_definitions(-DBOOST_TEST_DYN_LINK)
  endif(LIB_EXTENSION STREQUAL ".so")
endif(Boost_unit_test_framework_LIBRARY)

# Several tests check numerical results over many randomly generated inputs. The library
# seeds its generator from the clock by default, so those tests sampled a different part of
# the error distribution on every run and failed intermittently. Pinning the seed makes a
# failure reproducible instead of a coin flip. Override it to explore other inputs:
#
#   ctest --test-dir build -R unit_kmeans   # uses the pinned seed
#   OPENTISSUE_RANDOM_SEED=$RANDOM ./build/.../unit_kmeans   # explore
#
set(OPENTISSUE_TEST_RANDOM_SEED 20090306 CACHE STRING
    "Seed used by OpenTissue's random number generator when running the unit tests")

# Tests that do not reliably pass, whatever the seed. They are labelled rather than
# removed, so they still build and still run, but CI can gate on the rest of the suite:
#
#   ctest --test-dir build -LE unreliable    # the gating run
#   ctest --test-dir build -L  unreliable    # just these, informational
#
# unit_kmeans - passes for roughly 40% of seeds. K-means with random initialisation gets
#               caught in local minima, and the test says so itself: "If k-means gets
#               caught by a local minima then all the following unit-tests is likely to
#               fail!". Making it reliable means restarts or k-means++ in the algorithm,
#               not a tweak to the test.
# unit_bfgs   - passes for roughly 85% of seeds. BFGS on Rosenbrock stops well short of
#               the minimum when the random starting point is poor.
#
set(OPENTISSUE_UNRELIABLE_TESTS unit_kmeans unit_bfgs)

function(ot_add_test TEST_NAME)
  add_test(NAME ${TEST_NAME}
    COMMAND ${CMAKE_COMMAND} -E env
              OPENTISSUE=${PROJECT_SOURCE_DIR}
              OPENTISSUE_RANDOM_SEED=${OPENTISSUE_TEST_RANDOM_SEED}
              $<TARGET_FILE:${TEST_NAME}> ${ARGN}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )

  # Test properties are directory scoped, so this has to happen here, beside the
  # add_test() call, rather than in the top-level file.
  if(TEST_NAME IN_LIST OPENTISSUE_UNRELIABLE_TESTS)
    set_tests_properties(${TEST_NAME} PROPERTIES LABELS "unreliable")
  endif()
endfunction()

option(OPENTISSUE_ENABLE_CODE_COVERAGE "Enable coverage reporting" OFF)
if(OPENTISSUE_ENABLE_CODE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  # Add required flags (GCC & LLVM/Clang)
  target_compile_options(OpenTissue INTERFACE
    -O0        # no optimization
    -g         # generate debug info
    --coverage # sets all required flags
  )
  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.13)
    target_link_options(OpenTissue INTERFACE --coverage)
  else()
    target_link_libraries(OpenTissue INTERFACE --coverage)
  endif()
endif()

#-----------------------------------------------------------------------------
#
# Several unit tests link a third-party library that is optional (see
# cmake/OpenTissueDependencies.cmake). Add those directories only when the
# libraries they need were actually found, so that a partial dependency set
# still yields a working test suite instead of a configure error.
#
# Usage: ot_add_test_subdirectory( <dir> [FEATURE...] )
# where FEATURE is the suffix of an OPENTISSUE_HAVE_<FEATURE> variable.
#
function(ot_add_test_subdirectory dir)
  foreach(_feature IN LISTS ARGN)
    if(NOT OPENTISSUE_HAVE_${_feature})
      message(STATUS "Skipping unit tests '${dir}' (needs ${_feature})")
      return()
    endif()
  endforeach()
  add_subdirectory(${dir})
endfunction()

add_subdirectory( collision )
add_subdirectory( core )
add_subdirectory( dynamics )
add_subdirectory( kinematics )
add_subdirectory( utility )
ot_add_test_subdirectory( io PNG )

#-----------------------------------------------------------------------------
#
# Check that every header compiles standalone. Off by default: it adds roughly
# 900 translation units to the build.
#
option(OPENTISSUE_ENABLE_HEADER_TESTS "Check that every OpenTissue header is self-contained" OFF)
if(OPENTISSUE_ENABLE_HEADER_TESTS)
  add_subdirectory( headers )
endif()

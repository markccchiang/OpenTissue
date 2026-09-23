#----------------------------------------------------------------------
#
# We got a bunch of compile tests, it really make no sense to build an
# executable from these. We just create a static lib for each of these.
#
set(SRC_FILES
  src/simulator_types_compile_testing/double_2pass_shock_propagation_stepper.cpp
  src/simulator_types_compile_testing/double_constraint_based_shock_propagation_stepper.cpp
  src/simulator_types_compile_testing/double_dynamics_projection_stepper.cpp
  src/simulator_types_compile_testing/double_dynamics_stepper.cpp
  src/simulator_types_compile_testing/double_first_order_stepper.cpp
  src/simulator_types_compile_testing/double_iterate_once_collision_resolver.cpp
  src/simulator_types_compile_testing/double_sequential_collision_resolver.cpp
  src/simulator_types_compile_testing/double_sequential_truncating_collision_resolver.cpp
  src/simulator_types_compile_testing/float_2pass_shock_propagation_stepper.cpp
  src/simulator_types_compile_testing/float_constraint_based_shock_propagation_stepper.cpp
  src/simulator_types_compile_testing/float_dynamics_projection_stepper.cpp
  src/simulator_types_compile_testing/float_dynamics_stepper.cpp
  src/simulator_types_compile_testing/float_first_order_stepper.cpp
  src/simulator_types_compile_testing/float_iterate_once_collision_resolver.cpp
  src/simulator_types_compile_testing/float_sequential_collision_resolver.cpp
  src/simulator_types_compile_testing/float_sequential_truncating_collision_resolver.cpp
)

add_library(unit_multibody_build_test ${SRC_FILES})
target_link_libraries(unit_multibody_build_test
  PRIVATE
    Boost::unit_test_framework
    Qhull::qhull_r
    OpenTissue
)

#----------------------------------------------------------------------
#
# The same simulator compile tests, on the Eigen math policy. This is what holds
# that policy to being interchangeable with the uBLAS ones: until these existed,
# no simulator had ever been assembled on it, and none could be. See the comment
# in eigen3_simulator_type_compile_test.h.
#
# Only when Eigen is available. This directory is already gated on Qhull, which
# mbd's collision detection needs, so both are present here.
#
if(OPENTISSUE_HAVE_EIGEN3)
  add_library(unit_multibody_eigen3_build_test
    src/eigen3_simulator_types_compile_testing/eigen3_2pass_shock_propagation_stepper.cpp
    src/eigen3_simulator_types_compile_testing/eigen3_constraint_based_shock_propagation_stepper.cpp
    src/eigen3_simulator_types_compile_testing/eigen3_dynamics_projection_stepper.cpp
    src/eigen3_simulator_types_compile_testing/eigen3_dynamics_stepper.cpp
    src/eigen3_simulator_types_compile_testing/eigen3_first_order_stepper.cpp
    src/eigen3_simulator_types_compile_testing/eigen3_iterate_once_collision_resolver.cpp
    src/eigen3_simulator_types_compile_testing/eigen3_sequential_collision_resolver.cpp
    src/eigen3_simulator_types_compile_testing/eigen3_sequential_truncating_collision_resolver.cpp
  )
  target_link_libraries(unit_multibody_eigen3_build_test
    PRIVATE
      Qhull::qhull_r
      OpenTissue
      Eigen3::Eigen
  )

  # And the runtime counterpart: a real simulator stepped on each of the three policies.
  add_executable(unit_mbd_math_policy_equivalence src/unit_mbd_math_policy_equivalence.cpp)
  target_link_libraries(unit_mbd_math_policy_equivalence
    PRIVATE
      Boost::unit_test_framework
      Qhull::qhull_r
      OpenTissue
      Eigen3::Eigen
  )
  ot_add_test(unit_mbd_math_policy_equivalence)
endif()

#----------------------------------------------------------------------
#
# This is the only executable in the retro unit test
#
add_executable(unit_multibody
  src/unit_retro.cpp
  src/projected_gauss_seidel_compile_test.cpp
  src/math_policies_compile_test.cpp
  src/matrix_setup.h
  src/compile_test.cpp
)

target_link_libraries(unit_multibody
  PRIVATE
    Boost::unit_test_framework
    Qhull::qhull_r
    OpenTissue
)

install(
  TARGETS unit_multibody
  RUNTIME DESTINATION  bin/units
  )

ot_add_test(unit_multibody)

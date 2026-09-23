#ifndef EIGEN3_SIMULATOR_TYPE_COMPILE_TEST
#define EIGEN3_SIMULATOR_TYPE_COMPILE_TEST
//
// OpenTissue, A toolbox for physical based simulation and animation.
// Copyright (C) 2007 Department of Computer Science, University of Copenhagen
//
#include <OpenTissue/configuration.h>

#include "../simulator_types_compile_testing/simulator_type_compile_test.h"
#include <OpenTissue/dynamics/mbd/math/mbd_eigen3_math_policy.h>

//
// The same simulator compile tests as simulator_types_compile_testing/, with the Eigen math
// policy in place of the uBLAS one.
//
// The Eigen policy is meant to be interchangeable with the uBLAS ones, and these are what hold
// it to that. They exist because it was not: the policy's own operations were tested, and
// ProjectedGaussSeidel was driven through it, but no simulator was ever assembled on it -- and
// none could be. The code that builds the mass matrix, the Jacobian and the state vectors
// reached past the policy into uBLAS-only API (vector_type::size_type, .clear(), .empty(),
// writing into a sparse matrix with operator(), an unqualified prod found by argument-dependent
// lookup), so every one of these combinations failed to compile.
//
// Each file instantiates one stepper or collision resolver under every simulator it can be
// combined with, in both precisions.
//

/**
* The six simulators every stepper and collision resolver can be combined with.
*/
template<typename math_types, template <typename> class stepper_type>
void eigen3_common_simulators_compile_test()
{
  void (*ptr)() = 0;
  ptr = &(simulator_type_compile_test<math_types, stepper_type, OpenTissue::mbd::BisectionStepSimulator> );
  ptr = &(simulator_type_compile_test<math_types, stepper_type, OpenTissue::mbd::ExplicitFixedStepSimulator> );
  ptr = &(simulator_type_compile_test<math_types, stepper_type, OpenTissue::mbd::ExplicitSeparateErrorCorrectionFixedStepSimulator> );
  ptr = &(simulator_type_compile_test<math_types, stepper_type, OpenTissue::mbd::FixPointStepSimulator> );
  ptr = &(simulator_type_compile_test<math_types, stepper_type, OpenTissue::mbd::ImplicitFixedStepSimulator> );
  ptr = &(simulator_type_compile_test<math_types, stepper_type, OpenTissue::mbd::SemiImplicitFixedStepSimulator> );
  (void)ptr;
}

/**
* All seven. SeparatedCollisionContactFixedStepSimulator cannot be combined with the two
* shock-propagation steppers: it performs stack propagation itself, so the stepper and the
* simulator each give every body the same stack-analysis members and every use of them is
* ambiguous. The uBLAS compile tests exclude that pairing too (it is commented out in
* double_/float_*_shock_propagation_stepper.cpp), so those two files call
* eigen3_common_simulators_compile_test() instead.
*/
template<typename math_types, template <typename> class stepper_type>
void eigen3_all_simulators_compile_test()
{
  eigen3_common_simulators_compile_test<math_types, stepper_type>();

  void (*ptr)() = 0;
  ptr = &(simulator_type_compile_test<math_types, stepper_type, OpenTissue::mbd::SeparatedCollisionContactFixedStepSimulator> );
  (void)ptr;
}

// EIGEN3_SIMULATOR_TYPE_COMPILE_TEST
#endif

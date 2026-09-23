//
// OpenTissue, A toolbox for physical based simulation and animation.
// Copyright (C) 2007 Department of Computer Science, University of Copenhagen
//
#include <OpenTissue/configuration.h>

#include "eigen3_simulator_type_compile_test.h"

namespace
{
  template< typename types  >
  class eigen3_constraint_based_shock_propagation_stepper
    : public OpenTissue::mbd::ConstraintBasedShockPropagationStepper< types, OpenTissue::mbd::ProjectedGaussSeidel<typename types::math_policy> >
  {};
}

// Not combined with SeparatedCollisionContactFixedStepSimulator; see the header.
void (*eigen3_constraint_based_shock_propagation_stepper_double)() = &(eigen3_common_simulators_compile_test< OpenTissue::mbd::eigen3_math_policy<double>, eigen3_constraint_based_shock_propagation_stepper > );
void (*eigen3_constraint_based_shock_propagation_stepper_float)()  = &(eigen3_common_simulators_compile_test< OpenTissue::mbd::eigen3_math_policy<float>,  eigen3_constraint_based_shock_propagation_stepper > );

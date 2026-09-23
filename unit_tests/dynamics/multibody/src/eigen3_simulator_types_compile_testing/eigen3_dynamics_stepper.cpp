//
// OpenTissue, A toolbox for physical based simulation and animation.
// Copyright (C) 2007 Department of Computer Science, University of Copenhagen
//
#include <OpenTissue/configuration.h>

#include "eigen3_simulator_type_compile_test.h"

namespace
{
  template< typename types  >
  class eigen3_dynamics_stepper
    : public OpenTissue::mbd::DynamicsStepper< types, OpenTissue::mbd::ProjectedGaussSeidel<typename types::math_policy> >
  {};
}

void (*eigen3_dynamics_stepper_double)() = &(eigen3_all_simulators_compile_test< OpenTissue::mbd::eigen3_math_policy<double>, eigen3_dynamics_stepper > );
void (*eigen3_dynamics_stepper_float)()  = &(eigen3_all_simulators_compile_test< OpenTissue::mbd::eigen3_math_policy<float>,  eigen3_dynamics_stepper > );

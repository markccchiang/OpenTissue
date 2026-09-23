//
// OpenTissue, A toolbox for physical based simulation and animation.
// Copyright (C) 2007 Department of Computer Science, University of Copenhagen
//
#include <OpenTissue/configuration.h>

#include "eigen3_simulator_type_compile_test.h"

namespace
{
  template< typename types  >
  class eigen3_iterate_once_collision_resolver
    : public OpenTissue::mbd::IterateOnceCollisionResolver< types, OpenTissue::mbd::collision_laws::FrictionalNewtonCollisionLawPolicy >
  {};
}

void (*eigen3_iterate_once_collision_resolver_double)() = &(eigen3_all_simulators_compile_test< OpenTissue::mbd::eigen3_math_policy<double>, eigen3_iterate_once_collision_resolver > );
void (*eigen3_iterate_once_collision_resolver_float)()  = &(eigen3_all_simulators_compile_test< OpenTissue::mbd::eigen3_math_policy<float>,  eigen3_iterate_once_collision_resolver > );

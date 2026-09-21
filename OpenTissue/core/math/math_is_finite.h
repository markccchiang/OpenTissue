#ifndef OPENTISSUE_CORE_MATH_IS_FINITE_H
#define OPENTISSUE_CORE_MATH_IS_FINITE_H
//
// OpenTissue Template Library
// - A generic toolbox for physics-based modeling and simulation.
// Copyright (C) 2008 Department of Computer Science, University of Copenhagen.
//
// OTTL is licensed under zlib: http://opensource.org/licenses/zlib-license.php
//
#include <OpenTissue/configuration.h>

#ifdef WIN32
#include <float.h>
#else
#include <cmath>
#endif

namespace OpenTissue
{
  namespace math
  {

#ifdef WIN32
#define is_finite(val) (_finite(val)!=0)  ///< Is finite number test
#else
// finite() is a legacy BSD function that was removed from POSIX.1-2008 and is no longer
// declared by modern C libraries. std::isfinite is the standard C++11 replacement, and
// mirrors what math_is_number.h already does with std::isnan.
#define is_finite(val) (std::isfinite(val))  ///< Is finite number test
#endif

  } // namespace math

} // namespace OpenTissue

//OPENTISSUE_CORE_MATH_IS_FINITE_H
#endif

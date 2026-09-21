#ifndef	OPENTISSUE_UTILITY_UTILITY_GREATER_PTR_H
#define	OPENTISSUE_UTILITY_UTILITY_GREATER_PTR_H
//
// OpenTissue, A toolbox for physical based	simulation and animation.
// Copyright (C) 2007 Department of	Computer Science, University of	Copenhagen
//
#include <OpenTissue/configuration.h>

#include <functional>

namespace OpenTissue
{
  namespace utility
  {

    /**
    * Greater Than Test.
    * This is used for doing a greater than test on data
    * structures containing pointers to data and not instances.
    */
    template<class T>
    struct greater_ptr
    {
      // std::binary_function was removed in C++17. These three typedefs are all it ever
      // supplied, so they are kept here for any code that still reads them.
      typedef T     first_argument_type;
      typedef T     second_argument_type;
      typedef bool  result_type;

      bool operator()(const T& x, const T& y) const { return (*x)>(*y); }
    };

  } //End of namespace utility

} //End of namespace OpenTissue

// OPENTISSUE_UTILITY_UTILITY_GREATER_PTR_H
#endif

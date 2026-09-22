//
// OpenTissue, A toolbox for physical based simulation and animation.
// Copyright (C) 2007 Department of Computer Science, University of Copenhagen
//
#include <OpenTissue/configuration.h>

#include <OpenTissue/utility/utility_timer.h>

#define BOOST_AUTO_TEST_MAIN
#include <OpenTissue/utility/utility_push_boost_filter.h>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>
#include <boost/test/tools/floating_point_comparison.hpp>
#include <boost/test/test_tools.hpp>
#include <OpenTissue/utility/utility_pop_boost_filter.h>

#if defined(WIN32)
#  include <winbase.h>  // for Sleep()
#else
#  include <unistd.h>  // for sleep()
#endif

using namespace OpenTissue;

BOOST_AUTO_TEST_SUITE(opentissue_utility_timer);

  BOOST_AUTO_TEST_CASE(double_testing)
  {
    OpenTissue::utility::Timer<double> timer;
    timer.start();
#if defined(WIN32)
    Sleep( 2 * 1000 );
#else
    sleep ( 2 );
#endif
    timer.stop();
    double duration = timer();
    // sleep() guarantees it will not return early, but promises nothing about how late it
    // returns -- that is up to the scheduler. So the lower bound is a statement about the
    // Timer, while a tight upper bound would be a statement about how busy the machine is.
    // At 2.1 this failed on a loaded CI runner that took 2.11s to come back from a 2s sleep.
    // Three seconds still catches every way the Timer could actually be wrong (stopped
    // clock, wrong unit, uninitialised start) without depending on the host being idle.
    BOOST_CHECK( duration > 1.9);
    BOOST_CHECK( duration < 3.0);
  }

BOOST_AUTO_TEST_SUITE_END();

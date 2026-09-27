//
// OpenTissue, A toolbox for physical based simulation and animation.
// Copyright (C) 2007 Department of Computer Science, University of Copenhagen
//
#include <OpenTissue/configuration.h>

#include <OpenTissue/core/math/math_basic_types.h>
#include <OpenTissue/core/math/math_kmeans.h>

#define BOOST_AUTO_TEST_MAIN
#include <OpenTissue/utility/utility_push_boost_filter.h>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>
#include <boost/test/tools/floating_point_comparison.hpp>
#include <boost/test/test_tools.hpp>
#include <OpenTissue/utility/utility_pop_boost_filter.h>

#include <cmath>
#include <iostream>

using namespace OpenTissue;

BOOST_AUTO_TEST_SUITE(opentissue_math_kmeans);

// Whether to print each clustering. Off while sweeping many seeds, where it would bury the
// output of a failure.
bool verbose = true;

void four_groups_scenario()
{
  typedef OpenTissue::math::BasicMathTypes<double, size_t> math_types;
  typedef math_types::vector3_type                         vector3_type;
  typedef math_types::matrix3x3_type                       matrix3x3_type;
  typedef math_types::real_type                            real_type;
  typedef math_types::index_type                           index_type;
  typedef math_types::value_traits                         value_traits;
  typedef std::vector<vector3_type>                        vector_container;
  typedef std::vector<size_t>                              index_container;

  vector_container features;
  size_t index = 0;
  features.resize(40);
  real_type lower = - value_traits::half();
  real_type upper = value_traits::half();

  vector3_type center[4];
  center[0] = vector3_type( value_traits::half(), value_traits::half(), -value_traits::two() );
  center[1] = vector3_type( -value_traits::four(), value_traits::zero(), -value_traits::half() );
  center[2] = vector3_type( value_traits::half(), value_traits::four(), value_traits::half() );
  center[3] = vector3_type( value_traits::half(), -value_traits::two(), value_traits::four() );

  for(size_t i = 0;i<10;++i,++index)
  {
    OpenTissue::math::random( features[index], lower, upper );
    features[index] += center[0];
  }
  for(size_t i = 0;i<10;++i,++index)
  {
    OpenTissue::math::random( features[index], lower, upper );
    features[index] += center[1];
  }
  for(size_t i = 0;i<10;++i,++index)
  {
    OpenTissue::math::random( features[index], lower, upper );
    features[index] += center[2];
  }
  for(size_t i = 0;i<10;++i,++index)
  {
    OpenTissue::math::random( features[index], lower, upper );
    features[index] += center[3];
  }

  vector_container cluster_centers;
  index_container cluster_indexes;

  size_t K = 4;
  size_t iteration = 0u;
  size_t max_iterations = 50u;

  // K-means may get caught by local minimas, in such cases clusters
  // seem to ``melt'' together unexpected.
  //
  // If k-means gets caught by a local minima then all the following
  // unit-tests is likely to fail!
  //
  OpenTissue::math::kmeans( 
    features.begin()
    , features.end()
    , cluster_centers
    , cluster_indexes
    , K
    , iteration
    , max_iterations
    );

  BOOST_CHECK( iteration < max_iterations );

  for(size_t i = 0;i<40;++i)
    if(verbose) std::cout << cluster_indexes[i] << " ";
  if(verbose) std::cout << std::endl;

  size_t cluster_order[4];
  cluster_order[0] = cluster_indexes[0];
  cluster_order[1] = cluster_indexes[10];
  cluster_order[2] = cluster_indexes[20];
  cluster_order[3] = cluster_indexes[30];

  BOOST_CHECK( cluster_order[0] != cluster_order[1] );
  BOOST_CHECK( cluster_order[0] != cluster_order[2] );
  BOOST_CHECK( cluster_order[0] != cluster_order[3] );
  BOOST_CHECK( cluster_order[1] != cluster_order[2] );
  BOOST_CHECK( cluster_order[1] != cluster_order[3] );
  BOOST_CHECK( cluster_order[2] != cluster_order[3] );

  for(size_t i = 0;i<10;++i)
    BOOST_CHECK( cluster_indexes[i] ==  cluster_order[0] );
  for(size_t i = 10;i<20;++i)
    BOOST_CHECK( cluster_indexes[i] ==  cluster_order[1] );
  for(size_t i = 20;i<30;++i)
    BOOST_CHECK( cluster_indexes[i] ==  cluster_order[2] );
  for(size_t i = 30;i<40;++i)
    BOOST_CHECK( cluster_indexes[i] ==  cluster_order[3] );

  for(size_t c = 0;c<K;++c)
  {
    if(verbose) std::cout << cluster_centers[ cluster_order[c] ] << std::endl;
    real_type dist = OpenTissue::math::length( center[c] - cluster_centers[ cluster_order[c] ] );
    BOOST_CHECK(dist < value_traits::half() );
  }
}

BOOST_AUTO_TEST_CASE(simple_test)
{
  four_groups_scenario();
}

// The scenario above draws random feature points, and k-means random initial centers, and
// CI runs it for one pinned seed only. K-means once found these four groups for only about
// 27% of seeds. So also run it for a fixed range of seeds, reseeding the generator here,
// independent of the environment.
BOOST_AUTO_TEST_CASE(many_seeds)
{
  verbose = false;
  for(unsigned int seed = 1u; seed <= 50u; ++seed)
  {
    BOOST_TEST_CONTEXT("seed " << seed)
    {
      OpenTissue::math::Random<double>::seed(seed);
      four_groups_scenario();
    }
  }
  verbose = true;
}

BOOST_AUTO_TEST_CASE(no_cluster_is_left_empty)
{
  // A single run, no restarts: every cluster must still end up with feature points. The
  // centers were once placed at random in the bounding box, and a center that landed away
  // from the data kept an empty cluster for good.
  typedef OpenTissue::math::BasicMathTypes<double, size_t> math_types;
  typedef math_types::vector3_type                         vector3_type;
  typedef std::vector<vector3_type>                        vector_container;
  typedef std::vector<size_t>                              index_container;

  // Two tight groups far apart, with most of the bounding box empty, and more clusters
  // than groups.
  vector_container features;
  for(size_t i = 0; i < 20; ++i)
  {
    vector3_type p;
    OpenTissue::math::random( p, -0.1, 0.1 );
    features.push_back( p + vector3_type( i < 10 ? -10.0 : 10.0, 0.0, 0.0 ) );
  }

  size_t const K = 5;
  for(size_t trial = 0; trial < 50; ++trial)
  {
    vector_container centers;
    index_container  membership;
    size_t           iteration = 0u;
    OpenTissue::math::kmeans( features.begin(), features.end(), centers, membership, K, iteration, 50u, 1u );

    std::vector<size_t> count( K, 0u );
    for(size_t i = 0; i < membership.size(); ++i)
      ++count[ membership[i] ];
    for(size_t c = 0; c < K; ++c)
      BOOST_CHECK( count[c] > 0u );
  }
}

BOOST_AUTO_TEST_SUITE_END();

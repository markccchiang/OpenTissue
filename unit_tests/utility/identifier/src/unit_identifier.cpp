//
// OpenTissue, A toolbox for physical based simulation and animation.
// Copyright (C) 2007 Department of Computer Science, University of Copenhagen
//
#include <OpenTissue/configuration.h>

#include <OpenTissue/utility/utility_identifier.h>

#define BOOST_AUTO_TEST_MAIN
#include <OpenTissue/utility/utility_push_boost_filter.h>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>
#include <boost/test/test_tools.hpp>
#include <OpenTissue/utility/utility_pop_boost_filter.h>

#include <set>
#include <sstream>
#include <string>
#include <vector>

//
// Every mbd body derives from Identifier, which names each object "ID" followed by its unique
// index. It used to build that name as "ID" + m_index -- pointer arithmetic on the string
// literal, not concatenation -- so the first object got "ID", the second "D", the third ""
// and every later one read past the end of the literal. AddressSanitizer stopped on it the
// first time any multibody scene was built under it.
//

BOOST_AUTO_TEST_SUITE(opentissue_utility_identifier);

BOOST_AUTO_TEST_CASE(each_id_is_ID_followed_by_the_index)
{
  // Well past the third object, which is where the old code first read out of bounds.
  std::vector<OpenTissue::utility::Identifier> ids(10);

  for(size_t i = 0; i < ids.size(); ++i)
  {
    std::ostringstream expected;
    expected << "ID" << ids[i].get_index();
    BOOST_CHECK_EQUAL(ids[i].get_id(), expected.str());
  }
}

BOOST_AUTO_TEST_CASE(ids_and_indices_are_unique)
{
  std::vector<OpenTissue::utility::Identifier> ids(10);

  std::set<std::string> names;
  std::set<size_t>      indices;
  for(size_t i = 0; i < ids.size(); ++i)
  {
    names.insert(ids[i].get_id());
    indices.insert(ids[i].get_index());
  }
  BOOST_CHECK_EQUAL(names.size(),   ids.size());
  BOOST_CHECK_EQUAL(indices.size(), ids.size());
}

BOOST_AUTO_TEST_CASE(set_id_replaces_the_generated_name)
{
  OpenTissue::utility::Identifier id;
  id.set_id("pendulum");
  BOOST_CHECK_EQUAL(id.get_id(), std::string("pendulum"));
}

BOOST_AUTO_TEST_SUITE_END();

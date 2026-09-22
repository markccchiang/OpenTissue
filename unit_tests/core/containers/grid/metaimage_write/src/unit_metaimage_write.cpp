//
// OpenTissue, A toolbox for physical based simulation and animation.
// Copyright (C) 2007 Department of Computer Science, University of Copenhagen
//
#include <OpenTissue/configuration.h>

#include <OpenTissue/core/math/math_basic_types.h>
#include <OpenTissue/core/containers/grid/grid.h>
#include <OpenTissue/core/containers/grid/io/grid_metaimage_write.h>

#define BOOST_AUTO_TEST_MAIN
#include <OpenTissue/utility/utility_push_boost_filter.h>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>
#include <boost/test/tools/floating_point_comparison.hpp>
#include <boost/test/test_tools.hpp>
#include <OpenTissue/utility/utility_pop_boost_filter.h>

#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace
{
  /**
  * Read a MetaImage header back into key/value pairs, so the test can assert on what was
  * written rather than on the exact formatting.
  */
  std::map<std::string,std::string> read_header(std::string const & filename)
  {
    std::map<std::string,std::string> fields;
    std::ifstream in(filename.c_str());
    std::string line;
    while(std::getline(in, line))
    {
      std::string::size_type const eq = line.find('=');
      if(eq == std::string::npos)
        continue;
      std::string key   = line.substr(0, eq);
      std::string value = line.substr(eq + 1u);
      while(!key.empty()   && key[key.size()-1u]   == ' ') key.erase(key.size()-1u);
      while(!value.empty() && value[0]             == ' ') value.erase(0, 1u);
      while(!value.empty() && (value[value.size()-1u] == '\r' || value[value.size()-1u] == ' '))
        value.erase(value.size()-1u);
      fields[key] = value;
    }
    return fields;
  }
}

BOOST_AUTO_TEST_SUITE(opentissue_grid_metaimage_write);

BOOST_AUTO_TEST_CASE(header_describes_the_grid)
{
  typedef OpenTissue::math::BasicMathTypes<double, size_t>  math_types;
  typedef math_types::vector3_type                          vector3_type;
  typedef OpenTissue::grid::Grid<float, math_types>         grid_type;

  grid_type grid;
  grid.create(vector3_type(-1.0, -2.0, -3.0), vector3_type(1.0, 2.0, 3.0), 4u, 5u, 6u);

  // A value pattern that depends on all three indices, so a transposed or mis-strided
  // write would not round-trip.
  for(size_t k = 0u; k < grid.K(); ++k)
    for(size_t j = 0u; j < grid.J(); ++j)
      for(size_t i = 0u; i < grid.I(); ++i)
        grid(i,j,k) = static_cast<float>(i + 10u*j + 100u*k);

  BOOST_CHECK( OpenTissue::grid::metaimage_write("unit_metaimage", grid) );

  std::map<std::string,std::string> const fields = read_header("unit_metaimage.mhd");

  BOOST_CHECK_EQUAL( fields.count("ObjectType"),      1u );
  BOOST_CHECK_EQUAL( fields.find("ObjectType")->second, std::string("Image") );
  BOOST_CHECK_EQUAL( fields.find("NDims")->second,      std::string("3") );
  BOOST_CHECK_EQUAL( fields.find("BinaryData")->second, std::string("True") );
  BOOST_CHECK_EQUAL( fields.find("ElementType")->second, std::string("MET_FLOAT") );
  BOOST_CHECK_EQUAL( fields.find("DimSize")->second,     std::string("4 5 6") );

  // The data file must be a bare name: MetaImage resolves it relative to the header.
  BOOST_CHECK_EQUAL( fields.find("ElementDataFile")->second, std::string("unit_metaimage.raw") );

  // Offset is the grid's minimum corner.
  {
    std::istringstream ist(fields.find("Offset")->second);
    double x = 0.0, y = 0.0, z = 0.0;
    ist >> x >> y >> z;
    BOOST_CHECK_CLOSE( x, -1.0, 0.01 );
    BOOST_CHECK_CLOSE( y, -2.0, 0.01 );
    BOOST_CHECK_CLOSE( z, -3.0, 0.01 );
  }

  // Spacing is what the grid reports, not something recomputed.
  {
    std::istringstream ist(fields.find("ElementSpacing")->second);
    double x = 0.0, y = 0.0, z = 0.0;
    ist >> x >> y >> z;
    BOOST_CHECK_CLOSE( x, static_cast<double>(grid.dx()), 0.01 );
    BOOST_CHECK_CLOSE( y, static_cast<double>(grid.dy()), 0.01 );
    BOOST_CHECK_CLOSE( z, static_cast<double>(grid.dz()), 0.01 );
  }
}

BOOST_AUTO_TEST_CASE(raw_block_round_trips)
{
  typedef OpenTissue::math::BasicMathTypes<double, size_t>  math_types;
  typedef math_types::vector3_type                          vector3_type;
  typedef OpenTissue::grid::Grid<float, math_types>         grid_type;

  grid_type grid;
  grid.create(vector3_type(0.0, 0.0, 0.0), vector3_type(1.0, 1.0, 1.0), 4u, 5u, 6u);
  for(size_t k = 0u; k < grid.K(); ++k)
    for(size_t j = 0u; j < grid.J(); ++j)
      for(size_t i = 0u; i < grid.I(); ++i)
        grid(i,j,k) = static_cast<float>(i + 10u*j + 100u*k);

  BOOST_CHECK( OpenTissue::grid::metaimage_write("unit_metaimage_raw.mhd", grid) );

  std::ifstream raw("unit_metaimage_raw.raw", std::ios::binary);
  BOOST_REQUIRE( raw.is_open() );

  std::vector<float> values(grid.size(), 0.0f);
  raw.read(reinterpret_cast<char*>(&values[0]),
           static_cast<std::streamsize>(grid.size() * sizeof(float)));
  BOOST_CHECK_EQUAL( static_cast<size_t>(raw.gcount()), grid.size() * sizeof(float) );

  // i must vary fastest, which is the order both OpenTissue and MetaImage use.
  size_t n = 0u;
  for(size_t k = 0u; k < grid.K(); ++k)
    for(size_t j = 0u; j < grid.J(); ++j)
      for(size_t i = 0u; i < grid.I(); ++i, ++n)
        BOOST_CHECK_EQUAL( values[n], static_cast<float>(i + 10u*j + 100u*k) );
}

BOOST_AUTO_TEST_SUITE_END();

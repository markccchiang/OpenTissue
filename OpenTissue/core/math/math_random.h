#ifndef OPENTISSUE_CORE_MATH_MATH_RANDOM_H
#define OPENTISSUE_CORE_MATH_MATH_RANDOM_H
//
// OpenTissue Template Library
// - A generic toolbox for physics-based modeling and simulation.
// Copyright (C) 2008 Department of Computer Science, University of Copenhagen.
//
// OTTL is licensed under zlib: http://opensource.org/licenses/zlib-license.php
//
#include <OpenTissue/configuration.h>

#include <boost/cast.hpp> // Needed for boost::numeric_cast

#include <OpenTissue/core/math/math_constants.h>


#include <ctime>   // for std::time()
#include <cstdlib> // for std::getenv(), std::strtoul(), std::srand() and std::rand()

// Boost.Random, unconditionally.
//
// This used to be guarded by #ifdef BOOST_VERSION, and the class below was then chosen
// by #ifdef BOOST_RANDOM_HPP. BOOST_VERSION comes from <boost/version.hpp>, which this
// header never included, so whether Boost was used depended on whether some earlier
// include happened to pull Boost in first. Two different definitions of
// OpenTissue::math::Random therefore existed across translation units: an ODR violation,
// and one that showed up as the same function appearing at two different lines in the
// coverage data. Boost is a required dependency of OpenTissue, so there is no reason for
// the fallback to exist.
#include <boost/random.hpp>

namespace OpenTissue
{

  namespace math
  {

    namespace detail
    {
      /**
      * Seed used the first time a random generator is created.
      *
      * By default this is the wall clock, so a program gets different numbers on every
      * run. Setting the environment variable OPENTISSUE_RANDOM_SEED pins it instead,
      * which makes a run reproducible. The unit tests rely on this: several of them
      * check numerical results over many random inputs, and with a clock seed they
      * sample a different part of the error distribution on every run and fail
      * intermittently.
      */
      inline unsigned int initial_random_seed()
      {
        if(char const * env = std::getenv("OPENTISSUE_RANDOM_SEED"))
          return static_cast<unsigned int>(std::strtoul(env, 0, 10));
        return static_cast<unsigned int>(std::time(0));
      }
    } // namespace detail



    template<typename value_type>
    class Random
    {
    public:

      typedef boost::minstd_rand          generator_type;

    protected:

      static generator_type &  generator()
      {
        static generator_type tmp( detail::initial_random_seed() );
        return tmp;
      }

    public:

      /**
      * Reseed the shared generator, so a sequence can be reproduced on demand.
      */
      static void seed(unsigned int s) { generator().seed(s); }

      typedef value_type                                                     T;
      typedef boost::uniform_real<T>                                         distribution_type;
      typedef boost::variate_generator<generator_type&, distribution_type >  random_type;

      distribution_type       m_distribution;
      random_type             m_random;

    public:

      Random()
        : m_distribution((value_type) 0,(value_type) 1)
        , m_random(generator(), m_distribution)
      {}

      Random(T lower,T upper)
        : m_distribution(lower,upper)
        , m_random(generator(), m_distribution)
      {}

    private:

      Random(Random const & rnd){}
      Random & operator=(Random const & rnd){return *this;}

    public:

      T operator()() { return m_random();  }

      bool operator==(Random const & rnd) const { return m_distribution == rnd.m_distribution; }

    };


  } // namespace math

} // namespace OpenTissue

//OPENTISSUE_CORE_MATH_MATH_RANDOM_H
#endif

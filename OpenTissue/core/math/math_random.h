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

#ifdef BOOST_VERSION  //--- FIXME: Nicer way too see if we have boost?
#  include <boost/random.hpp>
#endif

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

#ifdef BOOST_RANDOM_HPP


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

#else


    template <typename value_type>
    class Random
    {
    protected:

      typedef value_type  T;
      typedef Random<T>   self;

      T m_lower;
      T m_upper;

    protected:

      static bool & is_initialized()
      {
        static bool initialized = false;
        return initialized;
      }

    public:

      Random() 
        : m_lower(math::detail::zero<T>()) 
        , m_upper(math::detail::one<T>())
      {
        if(!is_initialized())
        {
          std::srand( detail::initial_random_seed() );
          is_initialized() = true;
        }
      }

      Random(T lower,T upper) 
        : m_lower(lower) 
        , m_upper(upper)
      { 
        self();
      }

    private:

      Random(Random const & rnd){}
      Random & operator=(Random const & rnd){return *this;}

    public:

      T operator()() const
      {
        double rnd = rand()/(1.0*RAND_MAX);
        return boost::numeric_cast<T>(m_lower+(m_upper-m_lower)*rnd);
      }

      bool operator==(Random const & rnd) const { return (m_lower==rnd.m_lower && m_upper==rnd.m_upper);  }

    };

#endif

  } // namespace math

} // namespace OpenTissue

//OPENTISSUE_CORE_MATH_MATH_RANDOM_H
#endif

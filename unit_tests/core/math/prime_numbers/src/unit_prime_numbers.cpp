//
// OpenTissue, A toolbox for physical based simulation and animation.
// Copyright (C) 2007 Department of Computer Science, University of Copenhagen
//
#include <OpenTissue/configuration.h>

#include <OpenTissue/core/math/math_prime_numbers.h>

#define BOOST_AUTO_TEST_MAIN
#include <OpenTissue/utility/utility_push_boost_filter.h>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>
#include <boost/test/test_tools.hpp>
#include <OpenTissue/utility/utility_pop_boost_filter.h>

#include <climits>
#include <vector>

//
// prime_search() sizes the spatial hash table that mbd's broad phase, SPH and the versatile
// model use. It rested on a Miller-Rabin test that computed d*d in int, which overflows once
// n passes 46341, so above that it rejected nearly every prime. prime_search() then scanned
// upwards for a prime it could never find: prime_search(100000) did not return. A scene of
// that many bodies hung while setting up collision detection.
//
// trial_division() is exact, just slow, so it is the reference throughout.
//

using namespace OpenTissue::math;

namespace
{

  // Plain square-and-multiply in 64 bits, as a reference for modular_exponentiation().
  long long reference_power(long long a, long long b, long long n)
  {
    long long d = 1;
    a %= n;
    while(b > 0)
    {
      if(b & 1)
        d = (d * a) % n;
      a = (a * a) % n;
      b >>= 1;
    }
    return d;
  }

  int const limit = 200000;

  // Large primes, including INT_MAX = 2^31 - 1, the largest argument these functions take.
  int const large_primes[] = { 46349, 65537, 1000003, 100000007, 1000000007, 2147483647 };

  // Carmichael numbers fool the Fermat test for every base coprime to them; Miller-Rabin must
  // still see through them. Plus large composites past the old overflow point.
  int const hard_composites[] = { 561, 1105, 1729, 2465, 2821, 6601, 8911, 41041, 825265,
                                  1000001, 2147483645, 2147483641 };

} // anonymous namespace

BOOST_AUTO_TEST_SUITE(opentissue_math_prime_numbers);

BOOST_AUTO_TEST_CASE(trial_division_is_a_sound_reference)
{
  int const primes[]     = { 2, 3, 5, 7, 11, 13, 97, 7919 };
  int const composites[] = { 4, 9, 15, 91, 561, 7917 };
  for(int p : primes)     BOOST_CHECK(trial_division(p));
  for(int c : composites) BOOST_CHECK(!trial_division(c));
}

//
// A correct Miller-Rabin test can never call a prime composite: a prime has no witness. So
// this can be checked exactly, unlike the other direction, which is probabilistic.
//
BOOST_AUTO_TEST_CASE(miller_rabin_never_rejects_a_prime)
{
  int rejected = 0;
  int first_rejected = 0;
  for(int n = 3; n < limit; n += 2)
    if(trial_division(n) && !miller_rabin(n, 4))
    {
      if(rejected == 0)
        first_rejected = n;
      ++rejected;
    }
  BOOST_CHECK_MESSAGE(rejected == 0, rejected << " primes below " << limit
    << " were called composite, the first " << first_rejected);

  for(int p : large_primes)
    BOOST_CHECK_MESSAGE(miller_rabin(p, 4), p << " is prime but was called composite");
}

//
// The other direction. Each round lets a composite through with probability at most 1/4, and
// in practice far less, so this asks for nearly all of them rather than all of them.
//
BOOST_AUTO_TEST_CASE(miller_rabin_rejects_composites)
{
  int composites = 0;
  int accepted   = 0;
  for(int n = 9; n < limit; n += 2)
    if(!trial_division(n))
    {
      ++composites;
      if(miller_rabin(n, 4))
        ++accepted;
    }
  BOOST_CHECK_MESSAGE(accepted * 1000 < composites, accepted << " of " << composites
    << " odd composites below " << limit << " were called prime");

  for(int c : hard_composites)
    BOOST_CHECK_MESSAGE(!miller_rabin(c, 16), c << " is composite but was called prime");
}

BOOST_AUTO_TEST_CASE(modular_exponentiation_matches_a_64_bit_reference)
{
  int const moduli[] = { 7, 97, 46349, 65537, 1000003, 1000000007, 2147483647 };
  int const bases[]  = { 2, 3, 12345, 46341, 2147483646 };
  int const powers[] = { 0, 1, 2, 17, 65536, 1000000006 };

  for(int n : moduli)
    for(int a : bases)
      for(int b : powers)
        BOOST_CHECK_EQUAL(modular_exponentiation(a % n, b, n), reference_power(a, b, n));

  // Fermat: 2^(p-1) = 1 mod p, which is what pseudo_prime() relies on.
  for(int p : large_primes)
    BOOST_CHECK(pseudo_prime(p));
}

//
// prime_search(n) has to return the closest prime to n -- and has to return at all. The sizes
// here include ones it used to scan towards INT_MAX on; this test carries a CTest timeout so
// that a regression shows up as a failure rather than a hang.
//
BOOST_AUTO_TEST_CASE(prime_search_returns_the_closest_prime)
{
  int const sizes[] = { 10, 1000, 46000, 48000, 100000, 1000000, 10000000 };

  for(int n : sizes)
  {
    int const p = prime_search(n);
    BOOST_CHECK_MESSAGE(trial_division(p), "prime_search(" << n << ") = " << p << ", which is not prime");

    // No prime may be strictly closer to n than the one returned.
    int const distance = p > n ? p - n : n - p;
    for(int k = 0; k < distance; ++k)
    {
      BOOST_CHECK_MESSAGE(!(n - k > 1 && trial_division(n - k)), "prime_search(" << n << ") = " << p
        << " but " << n - k << " is closer");
      BOOST_CHECK_MESSAGE(!trial_division(n + k), "prime_search(" << n << ") = " << p
        << " but " << n + k << " is closer");
    }
  }
}

BOOST_AUTO_TEST_SUITE_END();

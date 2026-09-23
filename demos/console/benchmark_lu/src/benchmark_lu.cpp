//
// OpenTissue Template Library Demo
// - A specific demonstration of the flexibility of OTTL.
// Copyright (C) 2009 Department of Computer Science, University of Copenhagen.
//
// OTTL and OTTL Demos are licensed under zlib.
//
#include <OpenTissue/configuration.h>

#include <OpenTissue/core/math/big/big_lu.h>

#include <Eigen/Dense>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>

//
// Dense LU: OpenTissue's big::lu against Eigen's PartialPivLU on the same system.
//
// The one caller of big::lu that solves a large system is the variational interpolator
// (core/math/interpolation/interpolation_variational_interpolator.h), which factors a dense
// (N+4) x (N+4) matrix, N being the number of data points, once per fit.
//
// What it showed (Apple M-series, -O2): Eigen is 11-12x faster from N = 500 up -- 1.5 s
// against 0.13 s at N = 2000 -- to the same solution. That is a real gap, but the
// interpolator has no callers anywhere in OpenTissue and factors once per fit, so it did not
// justify porting core/math/big/ to Eigen. It is the number to look at again if that changes.
//
// The matrix is symmetric, like the interpolator's radial-basis system, and diagonally
// dominant, so that the comparison is about speed rather than conditioning.
//
// Build in Release. uBLAS leans on expression templates that are very slow unoptimised, so
// Debug timings say nothing about either library.
//

using clock_type = std::chrono::steady_clock;

static double milliseconds(clock_type::time_point a, clock_type::time_point b)
{
  return std::chrono::duration<double, std::milli>(b - a).count();
}

int main(int /*argc*/, char ** /*argv*/)
{
  std::printf("%6s | %12s %12s %8s | %s\n", "N", "big::lu ms", "Eigen ms", "speed", "max |x_ot - x_eigen|");

  for(int const n : {100, 250, 500, 1000, 2000})
  {
    ublas::matrix<double> A(n, n);
    ublas::vector<double> b(n);
    ublas::vector<double> x(n);
    Eigen::MatrixXd       E(n, n);
    Eigen::VectorXd       eb(n);

    for(int i = 0; i < n; ++i)
    {
      for(int j = 0; j < n; ++j)
      {
        double const value = (i == j)
          ? double(n)
          : std::cos(0.37 * i + 0.11 * j) * std::cos(0.37 * j + 0.11 * i);
        A(i, j) = value;
        E(i, j) = value;
      }
      b(i)  = std::sin(0.1 * i);
      eb(i) = b(i);
    }

    auto const t0 = clock_type::now();
    OpenTissue::math::big::lu(A, x, b);
    auto const t1 = clock_type::now();
    Eigen::VectorXd const ex = E.partialPivLu().solve(eb);
    auto const t2 = clock_type::now();

    double error = 0.0;
    for(int i = 0; i < n; ++i)
      error = std::max(error, std::fabs(x(i) - ex(i)));

    std::printf("%6d | %12.2f %12.2f %7.1fx | %.1e\n"
      , n, milliseconds(t0, t1), milliseconds(t1, t2), milliseconds(t0, t1) / milliseconds(t1, t2), error);
  }

  std::printf("\nSpeed is big::lu time over Eigen time; above 1x means Eigen was faster.\n");
  return 0;
}

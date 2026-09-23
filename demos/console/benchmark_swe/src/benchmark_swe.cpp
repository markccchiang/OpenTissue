//
// OpenTissue Template Library Demo
// - A specific demonstration of the flexibility of OTTL.
// Copyright (C) 2009 Department of Computer Science, University of Copenhagen.
//
// OTTL and OTTL Demos are licensed under zlib.
//
#include <OpenTissue/configuration.h>

#include <OpenTissue/dynamics/swe/swe_shallow_water_equation.h>

#include <Eigen/Sparse>
#include <Eigen/IterativeLinearSolvers>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <vector>

//
// Where does a shallow-water timestep spend its time, and would Eigen make the solve faster?
//
// This was written to decide whether porting core/math/big/ to Eigen was worth doing. The
// shallow-water solver is the only caller of big/ that solves a large system every timestep,
// so it is where such a port could have paid off.
//
// Each step is split into the phases ShallowWaterEquations::run() goes through, timed
// separately. The conjugate gradient solve is then repeated with Eigen on the identical
// matrix, right-hand side and starting guess, doing the same number of iterations -- so the
// comparison is per unit of work rather than per stopping rule, which the two libraries
// define differently.
//
// What it showed (Apple M-series, -O2):
//
//   * Eigen's CG is no faster: 0.8-0.9x, i.e. marginally slower. A sparse matrix-vector
//     product is bound by memory bandwidth, and uBLAS was already fine at it.
//   * The solve is not the bottleneck anyway. Assembling the matrix takes 50-65% of each step,
//     and that is the shallow-water code itself, not big/.
//
// Build in Release. uBLAS leans on expression templates that are very slow unoptimised, so
// Debug timings say nothing about either library.
//

using clock_type = std::chrono::steady_clock;

static double milliseconds(clock_type::time_point a, clock_type::time_point b)
{
  return std::chrono::duration<double, std::milli>(b - a).count();
}

/**
* Drives the solver's own protected steps in exactly the order run() calls them, so each phase
* can be timed on its own and the assembled system can be handed to Eigen as well.
*/
class Benchmark
  : public OpenTissue::swe::ShallowWaterEquations<double>
{
public:

  typedef Eigen::SparseMatrix<double, Eigen::RowMajor>   eigen_matrix_type;

  double m_setup     = 0.0;
  double m_assemble  = 0.0;
  double m_cg        = 0.0;
  double m_update    = 0.0;
  double m_eigen_cg  = 0.0;
  size_t m_iterations = 0;
  bool   m_mismatch   = false;

  /**
  * A Gaussian mound in the middle of a flat pool. Still water lets CG stop after a single
  * iteration, which would make the solve look free.
  */
  void disturb()
  {
    for(index_type x = 0; x < X; ++x)
      for(index_type y = 0; y < Y; ++y)
      {
        double const fx = double(x) / X - 0.5;
        double const fy = double(y) / Y - 0.5;
        h[x][y] = 2.0 + 0.5 * std::exp(-(fx * fx + fy * fy) / 0.005);
      }
  }

  void step(double const dt, bool const compare_with_eigen)
  {
    auto const t0 = clock_type::now();
    this->timestep = dt;
    compute_departure_points();
    compute_value_at_departure_points(h, h_tilde);
    compute_value_at_departure_points(u, u_tilde);
    compute_value_at_departure_points(v, v_tilde);
    compute_depth();

    auto const t1 = clock_type::now();
    assemble(A, rhs);

    auto const t2 = clock_type::now();
    vector_type const guess = hnew;   // run() warm-starts from the previous solution

    // run() uses the three-argument overload, which forwards to (15, 10e-4). The
    // six-argument form is called here only to read back the iteration count.
    size_t iterations = 0;
    OpenTissue::math::big::conjugate_gradient(A, hnew, rhs, 15u, 10e-4, iterations);
    m_iterations += iterations;

    auto const t3 = clock_type::now();
    if(compare_with_eigen)
      solve_with_eigen(guess, iterations);

    auto const t4 = clock_type::now();
    set_height(hnew);
    update_u();
    update_v();
    apply_velocity_constraints();
    nullify();

    auto const t5 = clock_type::now();
    m_setup    += milliseconds(t0, t1);
    m_assemble += milliseconds(t1, t2);
    m_cg       += milliseconds(t2, t3);
    m_update   += milliseconds(t4, t5);
  }

protected:

  void solve_with_eigen(vector_type const & guess, size_t const iterations)
  {
    // The copy is not timed: a real port would assemble straight into Eigen.
    std::vector< Eigen::Triplet<double> > triplets;
    for(auto row = A.begin1(); row != A.end1(); ++row)
      for(auto entry = row.begin(); entry != row.end(); ++entry)
        triplets.emplace_back(int(entry.index1()), int(entry.index2()), *entry);

    eigen_matrix_type E(A.size1(), A.size2());
    E.setFromTriplets(triplets.begin(), triplets.end());

    Eigen::VectorXd b(rhs.size());
    Eigen::VectorXd x0(guess.size());
    for(size_t i = 0; i < rhs.size(); ++i)
    {
      b(i)  = rhs(i);
      x0(i) = guess(i);
    }

    // No preconditioner, since OpenTissue's CG has none. OpenTissue counts the initial
    // residual as an iteration and Eigen does not, hence the minus one.
    Eigen::ConjugateGradient<eigen_matrix_type, Eigen::Lower | Eigen::Upper, Eigen::IdentityPreconditioner> cg;
    cg.setMaxIterations(iterations > 1 ? int(iterations - 1) : 1);
    cg.setTolerance(0.0);

    auto const t0 = clock_type::now();
    cg.compute(E);
    Eigen::VectorXd const x = cg.solveWithGuess(b, x0);
    auto const t1 = clock_type::now();
    m_eigen_cg += milliseconds(t0, t1);

    // A timing comparison is only meaningful if both did the same thing.
    double difference = 0.0;
    double magnitude  = 0.0;
    for(size_t i = 0; i < hnew.size(); ++i)
    {
      difference += (x(i) - hnew(i)) * (x(i) - hnew(i));
      magnitude  += hnew(i) * hnew(i);
    }
    if(std::sqrt(difference / magnitude) > 1e-6)
      m_mismatch = true;
  }
};

int main(int /*argc*/, char ** /*argv*/)
{
  int    const steps = 10;
  double const dt    = 0.01;

  std::printf("%6s %9s | %9s %9s %9s %9s | %7s %6s | %9s %6s\n"
    , "grid", "unknowns", "setup", "assemble", "CG", "update", "CG %", "iters", "Eigen CG", "speed");

  for(int const n : {64, 128, 256, 512})
  {
    {
      Benchmark warm_up;
      warm_up.init(n, n, 10.0, 10.0);
      warm_up.disturb();
      warm_up.step(dt, false);
    }

    Benchmark run;
    run.init(n, n, 10.0, 10.0);
    run.disturb();
    for(int k = 0; k < steps; ++k)
      run.step(dt, true);

    double const total = run.m_setup + run.m_assemble + run.m_cg + run.m_update;
    std::printf("%6d %9d | %9.2f %9.2f %9.2f %9.2f | %6.1f%% %6.1f | %9.2f %5.1fx%s\n"
      , n, n * n
      , run.m_setup / steps, run.m_assemble / steps, run.m_cg / steps, run.m_update / steps
      , 100.0 * run.m_cg / total, double(run.m_iterations) / steps
      , run.m_eigen_cg / steps, run.m_cg / run.m_eigen_cg
      , run.m_mismatch ? "  (solutions differ!)" : "");
  }

  std::printf("\nTimes are milliseconds per step, averaged over %d steps.\n", steps);
  std::printf("Speed is OpenTissue CG time over Eigen CG time; below 1x means Eigen was slower.\n");
  return 0;
}

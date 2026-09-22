//
// OpenTissue, A toolbox for physical based simulation and animation.
// Copyright (C) 2007 Department of Computer Science, University of Copenhagen
//
#include <OpenTissue/configuration.h>

#include <OpenTissue/dynamics/mbd/math/mbd_default_math_policy.h>
#include <OpenTissue/dynamics/mbd/math/mbd_eigen3_math_policy.h>
#include <OpenTissue/dynamics/mbd/solvers/mbd_projected_gauss_seidel.h>
#include <OpenTissue/core/math/math_constants.h>

#define BOOST_AUTO_TEST_MAIN
#include <OpenTissue/utility/utility_push_boost_filter.h>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>
#include <boost/test/tools/floating_point_comparison.hpp>
#include <boost/test/test_tools.hpp>
#include <OpenTissue/utility/utility_pop_boost_filter.h>

#include <cstddef>
#include <vector>

//
// The Eigen math policy is only useful if it agrees with the uBLAS one that mbd has always
// been built with. Rather than assert numbers worked out by hand, every case below runs the
// same problem through both policies and compares. That way the uBLAS policy acts as the
// reference implementation, which is exactly the relationship the two are supposed to have.
//

typedef OpenTissue::mbd::default_ublas_math_policy<double>  ublas_policy;
typedef OpenTissue::mbd::eigen3_math_policy<double>         eigen_policy;

typedef ublas_policy::size_type size_type;

namespace
{

  double const tolerance = 1e-12;

  //
  // One small, deliberately sparse test problem, shaped like something mbd would actually
  // produce: four constraint rows over two bodies of six degrees of freedom each.
  //
  size_type const rows    = 4;
  size_type const columns = 12;

  /**
  * Read one entry of a uBLAS sparse matrix.
  *
  * Two reasons not to write M(i,j) inline: the non-const operator() returns a proxy that
  * BOOST_CHECK_CLOSE cannot consume, and it inserts the element on access, which would
  * quietly change the matrix being compared. Going through a const reference does neither.
  */
  double at(ublas_policy::matrix_type const & M, size_type i, size_type j)
  {
    return M(i, j);
  }

  /**
  * The nonzero entries of the test Jacobian, as (row, column, value).
  *
  * Both policies are filled from this one list, through their own subrange() views, so the
  * comparison also covers the sparse write path rather than just the arithmetic.
  */
  struct Entry
  {
    size_type row;
    size_type column;
    double    value;
  };

  std::vector<Entry> jacobian_entries()
  {
    std::vector<Entry> entries;
    // Body A occupies columns 0..5, body B columns 6..11. The values are arbitrary but
    // fixed, and include a zero so that the explicit-zero insertion both policies perform
    // is exercised too.
    double const values[4][6] =
    {
        { -1.0,  0.0,  0.0,  0.0,  0.5, -0.25 }
      , {  0.0, -1.0,  0.0, -0.5,  0.0,  0.75 }
      , {  0.0,  0.0, -1.0,  0.25, -0.75, 0.0 }
      , {  0.3,  0.4,  0.5,  0.0,  0.0,  0.0  }
    };

    for(size_type i = 0; i < rows; ++i)
    {
      for(size_type j = 0; j < 6; ++j)
      {
        entries.push_back(Entry{i, j, values[i][j]});
        // Body B gets the negated block, as a contact or joint Jacobian would.
        entries.push_back(Entry{i, j + 6, -values[i][j]});
      }
    }
    return entries;
  }

  /**
  * Fill a policy's sparse matrix with the test Jacobian, writing through subrange() so the
  * matrix_range type is what actually performs the insertion.
  */
  template<typename policy>
  void fill_jacobian(typename policy::matrix_type & J)
  {
    policy::resize(J, rows, columns);

    std::vector<Entry> const entries = jacobian_entries();

    // Write in two blocks, one per body, the way mbd_get_jacobian_matrix.h does: a range of
    // rows by the six columns belonging to one body.
    for(size_type body = 0; body < 2; ++body)
    {
      typename policy::matrix_range block =
        policy::subrange(J, 0, rows, body * 6, body * 6 + 6);

      for(std::size_t k = 0; k < entries.size(); ++k)
      {
        Entry const & e = entries[k];
        if(e.column >= body * 6 && e.column < body * 6 + 6)
          block(e.row, e.column - body * 6) = e.value;
      }
    }
  }

  /**
  * A diagonal inverse-mass matrix, filled directly rather than through a range.
  */
  template<typename policy>
  void fill_inverse_mass(typename policy::matrix_type & W)
  {
    policy::resize(W, columns, columns);

    typename policy::matrix_range block = policy::subrange(W, 0, columns, 0, columns);
    for(size_type i = 0; i < columns; ++i)
      block(i, i) = 1.0 / (1.0 + static_cast<double>(i));
  }

  template<typename policy>
  void fill_vector(typename policy::vector_type & v, size_type n, double scale)
  {
    policy::resize(v, n);
    for(size_type i = 0; i < n; ++i)
      v(i) = scale * (1.0 + static_cast<double>(i) * 0.5);
  }

} // anonymous namespace

BOOST_AUTO_TEST_SUITE(opentissue_dynamics_mbd_eigen3_math_policy);

//
// The sparse write path. This is the part of the policy that could not simply forward to
// Eigen: a general rectangular block of an Eigen sparse matrix is read-only, so the policy
// supplies its own writable view. Check it puts values where uBLAS puts them.
//
BOOST_AUTO_TEST_CASE(sparse_matrix_range_writes_match_ublas)
{
  ublas_policy::matrix_type J_ublas;
  eigen_policy::matrix_type J_eigen;

  fill_jacobian<ublas_policy>(J_ublas);
  fill_jacobian<eigen_policy>(J_eigen);

  BOOST_CHECK_EQUAL(static_cast<size_type>(J_eigen.rows()), rows);
  BOOST_CHECK_EQUAL(static_cast<size_type>(J_eigen.cols()), columns);

  for(size_type i = 0; i < rows; ++i)
    for(size_type j = 0; j < columns; ++j)
      BOOST_CHECK_CLOSE(at(J_ublas, i, j), J_eigen.coeff(i, j), tolerance);
}

//
// The range must also report its own shape, because every constraint asserts on it before
// writing -- see BallJoint::get_linear_jacobian_A().
//
BOOST_AUTO_TEST_CASE(sparse_matrix_range_reports_its_shape)
{
  eigen_policy::matrix_type J;
  eigen_policy::resize(J, rows, columns);

  eigen_policy::matrix_range block = eigen_policy::subrange(J, 1, 4, 6, 9);

  BOOST_CHECK_EQUAL(block.size1(), static_cast<size_type>(3));
  BOOST_CHECK_EQUAL(block.size2(), static_cast<size_type>(3));

  block(0, 0) = 7.0;
  BOOST_CHECK_CLOSE(J.coeff(1, 6), 7.0, tolerance);
  BOOST_CHECK_CLOSE(block(0, 0), 7.0, tolerance);
}

BOOST_AUTO_TEST_CASE(vector_and_index_subranges_match_ublas)
{
  ublas_policy::vector_type v_ublas;
  eigen_policy::vector_type v_eigen;

  fill_vector<ublas_policy>(v_ublas, columns, 1.0);
  fill_vector<eigen_policy>(v_eigen, columns, 1.0);

  ublas_policy::vector_range r_ublas = ublas_policy::subrange(v_ublas, 3, 9);
  eigen_policy::vector_range r_eigen = eigen_policy::subrange(v_eigen, 3, 9);

  BOOST_CHECK_EQUAL(static_cast<size_type>(r_eigen.size()), static_cast<size_type>(6));

  for(size_type i = 0; i < 6; ++i)
    BOOST_CHECK_CLOSE(r_ublas(i), r_eigen(i), tolerance);

  // Writing through the range must reach the underlying vector.
  r_eigen(0) = -42.0;
  BOOST_CHECK_CLOSE(v_eigen(3), -42.0, tolerance);

  ublas_policy::idx_vector_type iv_ublas;
  eigen_policy::idx_vector_type iv_eigen;
  ublas_policy::resize(iv_ublas, columns);
  eigen_policy::resize(iv_eigen, columns);
  for(size_type i = 0; i < columns; ++i)
  {
    iv_ublas(i) = i;
    iv_eigen(i) = i;
  }

  ublas_policy::idx_vector_range ir_ublas = ublas_policy::subrange(iv_ublas, 2, 5);
  eigen_policy::idx_vector_range ir_eigen = eigen_policy::subrange(iv_eigen, 2, 5);
  for(size_type i = 0; i < 3; ++i)
    BOOST_CHECK_EQUAL(ir_ublas(i), ir_eigen(i));
}

BOOST_AUTO_TEST_CASE(dimensions_match_ublas)
{
  ublas_policy::matrix_type J_ublas;
  eigen_policy::matrix_type J_eigen;
  fill_jacobian<ublas_policy>(J_ublas);
  fill_jacobian<eigen_policy>(J_eigen);

  size_type m_ublas = 0, n_ublas = 0, m_eigen = 0, n_eigen = 0;
  ublas_policy::get_dimensions(J_ublas, m_ublas, n_ublas);
  eigen_policy::get_dimensions(J_eigen, m_eigen, n_eigen);

  BOOST_CHECK_EQUAL(m_ublas, m_eigen);
  BOOST_CHECK_EQUAL(n_ublas, n_eigen);

  ublas_policy::vector_type x_ublas;
  eigen_policy::vector_type x_eigen;
  fill_vector<ublas_policy>(x_ublas, columns, 1.0);
  fill_vector<eigen_policy>(x_eigen, columns, 1.0);

  size_type k_ublas = 0, k_eigen = 0;
  ublas_policy::get_dimension(x_ublas, k_ublas);
  eigen_policy::get_dimension(x_eigen, k_eigen);
  BOOST_CHECK_EQUAL(k_ublas, k_eigen);
}

//
// Every product the policy exposes, run through both implementations.
//
BOOST_AUTO_TEST_CASE(products_match_ublas)
{
  ublas_policy::matrix_type J_ublas;
  eigen_policy::matrix_type J_eigen;
  fill_jacobian<ublas_policy>(J_ublas);
  fill_jacobian<eigen_policy>(J_eigen);

  ublas_policy::vector_type x_ublas, b_ublas, y_ublas, r_ublas;
  eigen_policy::vector_type x_eigen, b_eigen, y_eigen, r_eigen;

  fill_vector<ublas_policy>(x_ublas, columns, 1.0);
  fill_vector<eigen_policy>(x_eigen, columns, 1.0);
  fill_vector<ublas_policy>(b_ublas, rows, 2.0);
  fill_vector<eigen_policy>(b_eigen, rows, 2.0);

  double const s = 0.375;

  // y = J x
  ublas_policy::prod(J_ublas, x_ublas, y_ublas);
  eigen_policy::prod(J_eigen, x_eigen, y_eigen);
  for(size_type i = 0; i < rows; ++i)
    BOOST_CHECK_CLOSE(y_ublas(i), y_eigen(i), tolerance);

  // y = J x + b
  ublas_policy::prod(J_ublas, x_ublas, b_ublas, y_ublas);
  eigen_policy::prod(J_eigen, x_eigen, b_eigen, y_eigen);
  for(size_type i = 0; i < rows; ++i)
    BOOST_CHECK_CLOSE(y_ublas(i), y_eigen(i), tolerance);

  // y = J x - b
  ublas_policy::prod_minus(J_ublas, x_ublas, b_ublas, y_ublas);
  eigen_policy::prod_minus(J_eigen, x_eigen, b_eigen, y_eigen);
  for(size_type i = 0; i < rows; ++i)
    BOOST_CHECK_CLOSE(y_ublas(i), y_eigen(i), tolerance);

  // y = (J x) * s
  ublas_policy::prod(J_ublas, x_ublas, y_ublas, s);
  eigen_policy::prod(J_eigen, x_eigen, y_eigen, s);
  for(size_type i = 0; i < rows; ++i)
    BOOST_CHECK_CLOSE(y_ublas(i), y_eigen(i), tolerance);

  // y += J x, starting from a known non-zero y
  fill_vector<ublas_policy>(y_ublas, rows, 3.0);
  fill_vector<eigen_policy>(y_eigen, rows, 3.0);
  ublas_policy::prod_add(J_ublas, x_ublas, y_ublas);
  eigen_policy::prod_add(J_eigen, x_eigen, y_eigen);
  for(size_type i = 0; i < rows; ++i)
    BOOST_CHECK_CLOSE(y_ublas(i), y_eigen(i), tolerance);

  // y += (J x) * s
  fill_vector<ublas_policy>(y_ublas, rows, 3.0);
  fill_vector<eigen_policy>(y_eigen, rows, 3.0);
  ublas_policy::prod_add(J_ublas, x_ublas, y_ublas, s);
  eigen_policy::prod_add(J_eigen, x_eigen, y_eigen, s);
  for(size_type i = 0; i < rows; ++i)
    BOOST_CHECK_CLOSE(y_ublas(i), y_eigen(i), tolerance);

  // r = J^T y
  fill_vector<ublas_policy>(y_ublas, rows, 3.0);
  fill_vector<eigen_policy>(y_eigen, rows, 3.0);
  ublas_policy::prod_trans(J_ublas, y_ublas, r_ublas);
  eigen_policy::prod_trans(J_eigen, y_eigen, r_eigen);
  for(size_type i = 0; i < columns; ++i)
    BOOST_CHECK_CLOSE(r_ublas(i), r_eigen(i), tolerance);

  // r = J^T y + c
  ublas_policy::vector_type c_ublas;
  eigen_policy::vector_type c_eigen;
  fill_vector<ublas_policy>(c_ublas, columns, 0.5);
  fill_vector<eigen_policy>(c_eigen, columns, 0.5);
  ublas_policy::prod_trans(J_ublas, y_ublas, c_ublas, r_ublas);
  eigen_policy::prod_trans(J_eigen, y_eigen, c_eigen, r_eigen);
  for(size_type i = 0; i < columns; ++i)
    BOOST_CHECK_CLOSE(r_ublas(i), r_eigen(i), tolerance);

  // x *= s
  fill_vector<ublas_policy>(x_ublas, columns, 1.0);
  fill_vector<eigen_policy>(x_eigen, columns, 1.0);
  ublas_policy::prod(x_ublas, s);
  eigen_policy::prod(x_eigen, s);
  for(size_type i = 0; i < columns; ++i)
    BOOST_CHECK_CLOSE(x_ublas(i), x_eigen(i), tolerance);

  // x = -y
  ublas_policy::assign_minus(y_ublas, x_ublas);
  eigen_policy::assign_minus(y_eigen, x_eigen);
  for(size_type i = 0; i < rows; ++i)
    BOOST_CHECK_CLOSE(x_ublas(i), x_eigen(i), tolerance);
}

//
// A = J W J^T, the Schur complement the velocity-level solver actually works on. This is the
// one sparse-times-sparse product in the policy.
//
BOOST_AUTO_TEST_CASE(compute_system_matrix_matches_ublas)
{
  ublas_policy::matrix_type J_ublas, W_ublas;
  eigen_policy::matrix_type J_eigen, W_eigen;

  fill_jacobian<ublas_policy>(J_ublas);
  fill_jacobian<eigen_policy>(J_eigen);
  fill_inverse_mass<ublas_policy>(W_ublas);
  fill_inverse_mass<eigen_policy>(W_eigen);

  ublas_policy::system_matrix_type A_ublas;
  eigen_policy::system_matrix_type A_eigen;

  ublas_policy::compute_system_matrix(W_ublas, J_ublas, A_ublas);
  eigen_policy::compute_system_matrix(W_eigen, J_eigen, A_eigen);

  BOOST_CHECK_EQUAL(static_cast<size_type>(A_eigen.m_A.rows()), rows);
  BOOST_CHECK_EQUAL(static_cast<size_type>(A_eigen.m_A.cols()), rows);

  for(size_type i = 0; i < rows; ++i)
    for(size_type j = 0; j < rows; ++j)
      BOOST_CHECK_CLOSE(at(A_ublas, i, j), A_eigen.m_A.coeff(i, j), tolerance);

  // ProjectedGaussSeidel divides by A(i,i), so the operator the policy adds for that has to
  // agree with uBLAS too -- including the cached diagonal it answers from.
  for(size_type i = 0; i < rows; ++i)
    BOOST_CHECK_CLOSE(at(A_ublas, i, i), A_eigen(i, i), tolerance);
}

//
// row_prod() is what the Gauss-Seidel sweep calls once per row per iteration, so it is the
// hottest thing in the policy and the easiest to get wrong -- it is the only operation that
// walks the sparse storage by hand.
//
BOOST_AUTO_TEST_CASE(row_prod_matches_ublas)
{
  ublas_policy::matrix_type J_ublas, W_ublas;
  eigen_policy::matrix_type J_eigen, W_eigen;

  fill_jacobian<ublas_policy>(J_ublas);
  fill_jacobian<eigen_policy>(J_eigen);
  fill_inverse_mass<ublas_policy>(W_ublas);
  fill_inverse_mass<eigen_policy>(W_eigen);

  ublas_policy::system_matrix_type A_ublas;
  eigen_policy::system_matrix_type A_eigen;
  ublas_policy::compute_system_matrix(W_ublas, J_ublas, A_ublas);
  eigen_policy::compute_system_matrix(W_eigen, J_eigen, A_eigen);

  ublas_policy::vector_type x_ublas;
  eigen_policy::vector_type x_eigen;
  fill_vector<ublas_policy>(x_ublas, rows, 1.5);
  fill_vector<eigen_policy>(x_eigen, rows, 1.5);

  for(size_type i = 0; i < rows; ++i)
  {
    double const a = ublas_policy::row_prod(A_ublas, i, x_ublas);
    double const b = eigen_policy::row_prod(A_eigen, i, x_eigen);
    BOOST_CHECK_CLOSE(a, b, tolerance);
  }
}

//
// The point of the whole exercise: drive the real solver, unmodified, through both policies
// and check it reaches the same answer. If this passes, the policy seam holds.
//
BOOST_AUTO_TEST_CASE(projected_gauss_seidel_matches_ublas)
{
  typedef OpenTissue::mbd::ProjectedGaussSeidel<ublas_policy> ublas_solver_type;
  typedef OpenTissue::mbd::ProjectedGaussSeidel<eigen_policy> eigen_solver_type;

  ublas_policy::matrix_type J_ublas, W_ublas;
  eigen_policy::matrix_type J_eigen, W_eigen;

  fill_jacobian<ublas_policy>(J_ublas);
  fill_jacobian<eigen_policy>(J_eigen);
  fill_inverse_mass<ublas_policy>(W_ublas);
  fill_inverse_mass<eigen_policy>(W_eigen);

  ublas_policy::vector_type gamma_u, b_u, lo_u, hi_u, mu_u, x_u;
  eigen_policy::vector_type gamma_e, b_e, lo_e, hi_e, mu_e, x_e;

  ublas_policy::resize(gamma_u, rows);
  eigen_policy::resize(gamma_e, rows);

  fill_vector<ublas_policy>(b_u, rows, -1.0);
  fill_vector<eigen_policy>(b_e, rows, -1.0);

  ublas_policy::resize(lo_u, rows);
  eigen_policy::resize(lo_e, rows);
  ublas_policy::resize(hi_u, rows);
  eigen_policy::resize(hi_e, rows);
  ublas_policy::resize(mu_u, rows);
  eigen_policy::resize(mu_e, rows);
  ublas_policy::resize(x_u, rows);
  eigen_policy::resize(x_e, rows);

  ublas_policy::idx_vector_type pi_u;
  eigen_policy::idx_vector_type pi_e;
  ublas_policy::resize(pi_u, rows);
  eigen_policy::resize(pi_e, rows);

  for(size_type i = 0; i < rows; ++i)
  {
    // Plain box bounds. pi holds the index of the normal force a friction row depends on;
    // the highest representable index is the "no dependency" sentinel that the joints
    // themselves write (see BallJoint::get_dependency_indices()). Without it every row is
    // treated as a friction row and gets bounds of +/-mu*x, which with mu = 0 pins the
    // whole solution to zero and makes the comparison below vacuous.
    lo_u(i) = lo_e(i) = 0.0;
    hi_u(i) = hi_e(i) = 10.0;
    mu_u(i) = mu_e(i) = 0.0;
    pi_u(i) = pi_e(i) = OpenTissue::math::detail::highest<size_type>();
  }

  ublas_solver_type ublas_solver;
  eigen_solver_type eigen_solver;
  ublas_solver.set_max_iterations(25);
  eigen_solver.set_max_iterations(25);

  ublas_solver.run(J_ublas, W_ublas, gamma_u, b_u, lo_u, hi_u, pi_u, mu_u, x_u);
  eigen_solver.run(J_eigen, W_eigen, gamma_e, b_e, lo_e, hi_e, pi_e, mu_e, x_e);

  // The solution must be non-trivial, or the comparison below would prove nothing.
  double magnitude = 0.0;
  for(size_type i = 0; i < rows; ++i)
    magnitude += x_u(i) * x_u(i);
  BOOST_CHECK(magnitude > 1e-6);

  for(size_type i = 0; i < rows; ++i)
    BOOST_CHECK_CLOSE(x_u(i), x_e(i), tolerance);
}

BOOST_AUTO_TEST_SUITE_END();

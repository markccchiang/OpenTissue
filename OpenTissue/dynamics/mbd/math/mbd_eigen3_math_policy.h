#ifndef OPENTISSUE_DYNAMICS_MBD_MATH_MBD_EIGEN3_MATH_POLICY_H
#define OPENTISSUE_DYNAMICS_MBD_MATH_MBD_EIGEN3_MATH_POLICY_H
//
// OpenTissue Template Library
// - A generic toolbox for physics-based modeling and simulation.
// Copyright (C) 2008 Department of Computer Science, University of Copenhagen.
//
// OTTL is licensed under zlib: http://opensource.org/licenses/zlib-license.php
//
#include <OpenTissue/configuration.h>

#include <OpenTissue/core/math/math_basic_types.h>

#include <Eigen/Core>
#include <Eigen/SparseCore>

#include <cassert>
#include <cstddef>

namespace OpenTissue
{
  namespace mbd
  {

    namespace detail
    {

      /**
      * A writable rectangular view onto a sparse matrix.
      *
      * This exists because Eigen and uBLAS disagree about sub-matrices of sparse
      * matrices, and mbd depends on the uBLAS behaviour.
      *
      * mbd assembles the Jacobian by handing each constraint a sub-block of J and
      * letting it write its own rows element by element -- see
      * mbd_get_jacobian_matrix.h and, for instance, BallJoint::get_linear_jacobian_A(),
      * which writes a dense 3x3 identity, explicit zeros included.
      *
      * ublas::matrix_range supports exactly that. Eigen's Block expression does not:
      * a block of a SparseMatrix is read-only unless it happens to be an "inner panel"
      * (whole columns of a column-major matrix, whole rows of a row-major one), and the
      * blocks used here are neither -- they are a few rows by three columns, starting at
      * whatever offset the body occupies.
      *
      * So the view is written by hand. It stores the matrix and the offsets and
      * forwards element access to coeffRef(), which inserts on first write just as
      * ublas::compressed_matrix::operator() does. See the note on reserve() in
      * eigen3_math_policy::resize() for why that is not as slow as it sounds.
      */
      template<typename matrix_type_>
      class SparseMatrixRange
      {
      public:

        typedef matrix_type_                           matrix_type;
        typedef typename matrix_type::Scalar           value_type;
        typedef std::size_t                            size_type;

      protected:

        matrix_type * m_A;         ///< The matrix this is a view onto. Never null in practice.
        size_type     m_row_begin; ///< Index of the first row of the view within m_A.
        size_type     m_col_begin; ///< Index of the first column of the view within m_A.
        size_type     m_rows;      ///< Number of rows in the view.
        size_type     m_cols;      ///< Number of columns in the view.

      public:

        SparseMatrixRange()
          : m_A(0)
          , m_row_begin(0)
          , m_col_begin(0)
          , m_rows(0)
          , m_cols(0)
        {}

        SparseMatrixRange(
            matrix_type & A
          , size_type row_begin
          , size_type row_end
          , size_type col_begin
          , size_type col_end
          )
          : m_A(&A)
          , m_row_begin(row_begin)
          , m_col_begin(col_begin)
          , m_rows(row_end - row_begin)
          , m_cols(col_end - col_begin)
        {
          assert(row_begin <= row_end || !"SparseMatrixRange(): empty row range");
          assert(col_begin <= col_end || !"SparseMatrixRange(): empty column range");
          assert(static_cast<Eigen::Index>(row_end) <= A.rows()
            || !"SparseMatrixRange(): row range out of bounds");
          assert(static_cast<Eigen::Index>(col_end) <= A.cols()
            || !"SparseMatrixRange(): column range out of bounds");
        }

        size_type size1() const { return m_rows; }
        size_type size2() const { return m_cols; }

        value_type & operator()(size_type i, size_type j)
        {
          assert(m_A                  || !"SparseMatrixRange(): no matrix");
          assert(i < m_rows           || !"SparseMatrixRange(): row index out of bounds");
          assert(j < m_cols           || !"SparseMatrixRange(): column index out of bounds");
          return m_A->coeffRef(
              static_cast<Eigen::Index>(m_row_begin + i)
            , static_cast<Eigen::Index>(m_col_begin + j)
            );
        }

        value_type operator()(size_type i, size_type j) const
        {
          assert(m_A                  || !"SparseMatrixRange(): no matrix");
          assert(i < m_rows           || !"SparseMatrixRange(): row index out of bounds");
          assert(j < m_cols           || !"SparseMatrixRange(): column index out of bounds");
          return m_A->coeff(
              static_cast<Eigen::Index>(m_row_begin + i)
            , static_cast<Eigen::Index>(m_col_begin + j)
            );
        }

      };

    } // namespace detail

    /**
    * An Eigen-backed math policy for the mbd engine.
    *
    * This is a drop-in alternative to default_ublas_math_policy: it offers the same
    * types and the same operations, so the mbd engine itself needs no changes to use
    * it. Pick one or the other when binding the types of a simulator.
    *
    * The motivation is speed. uBLAS has no vectorisation and its sparse products are
    * slow enough that OpenTissue carries hand-written replacements for them in
    * core/math/big/big_prod*.h. Eigen's products are the reason to prefer this policy.
    *
    * Note that Eigen3::Eigen requires C++14, whereas the rest of OpenTissue compiles as
    * far back as C++11. Including this header therefore raises the standard your
    * translation unit needs; nothing else in OpenTissue does that, which is why Eigen is
    * not linked into the OpenTissue::headers interface target.
    */
    template< typename real_type_ >
    class eigen3_math_policy
      : public OpenTissue::math::BasicMathTypes<real_type_,size_t>
    {
    public:
      typedef typename OpenTissue::math::BasicMathTypes<real_type_, size_t>   basic_math_types;
      typedef typename basic_math_types::real_type                            real_type;
      typedef typename basic_math_types::index_type                           index_type;
      typedef typename basic_math_types::value_traits                         value_traits;

    public:

      // Row-major, to match the layout ublas::compressed_matrix uses by default and
      // because row_prod() below walks one row at a time for the Gauss-Seidel solver.
      typedef Eigen::SparseMatrix<real_type_, Eigen::RowMajor>   matrix_type;
      typedef Eigen::Matrix<real_type_, Eigen::Dynamic, 1>       vector_type;
      typedef std::size_t                                        size_type;
      typedef Eigen::Matrix<index_type, Eigen::Dynamic, 1>       idx_vector_type;
      typedef Eigen::VectorBlock<vector_type>                    vector_range;
      typedef detail::SparseMatrixRange<matrix_type>             matrix_range;
      typedef Eigen::VectorBlock<idx_vector_type>                idx_vector_range;

      /**
      * The matrix the Gauss-Seidel sweep runs on: A = J W J^T, plus its diagonal.
      *
      * This is a class rather than a plain matrix_type because ProjectedGaussSeidel
      * divides by the diagonal entry through A(i,i) -- see mbd_projected_gauss_seidel.h --
      * and Eigen's sparse matrix has no operator(). The uBLAS policies both satisfy that
      * call: the default one inherits it from ublas::compressed_matrix, and the optimized
      * one hand-writes it. So operator() is part of what a math policy has to provide, and
      * this supplies it the same way the optimized policy does.
      *
      * The diagonal is cached in a dense vector because the solver reads it once per row
      * per iteration, and looking an entry up in a sparse row is a search rather than an
      * indexing operation.
      */
      class system_matrix_type
      {
      public:

        matrix_type m_A;   ///< The assembled system matrix, J W J^T.
        vector_type m_d;   ///< Its diagonal, cached by compute_system_matrix().

        real_type operator()(size_type i, size_type j) const
        {
          if(i == j)
          {
            assert(static_cast<size_type>(m_d.size()) > i
              || !"system_matrix_type::operator(i,j): i out of bound");
            return m_d(static_cast<Eigen::Index>(i));
          }
          return m_A.coeff(static_cast<Eigen::Index>(i), static_cast<Eigen::Index>(j));
        }

      };

    protected:

      typedef Eigen::Index                        eigen_index_type;

      static eigen_index_type idx(size_type i)
      {
        return static_cast<eigen_index_type>(i);
      }

      /**
      * Grow y to n entries if it is not that size already.
      *
      * The uBLAS policy calls resize(n,false) unconditionally before every product.
      * That is only safe because uBLAS treats a resize to the current size as a no-op
      * and so leaves the contents alone -- which the accumulating products (y += ...)
      * rely on. Resizing only when the size actually differs makes that explicit
      * rather than incidental.
      */
      static void ensure_size(vector_type & y, size_type n)
      {
        if(static_cast<size_type>(y.size()) != n)
          y.setZero(idx(n));
      }

    public:

      static matrix_range subrange(
          matrix_type & M
        , size_type start1
        , size_type stop1
        , size_type start2
        , size_type stop2
        )
      {
        return matrix_range(M, start1, stop1, start2, stop2);
      }

      static vector_range subrange(vector_type & v, size_type start, size_type stop )
      {
        return v.segment(idx(start), idx(stop - start));
      }

      static idx_vector_range subrange(idx_vector_type & v, size_type start, size_type stop )
      {
        return v.segment(idx(start), idx(stop - start));
      }

      static void get_dimension(vector_type const & v, size_type & n)
      {
        n = static_cast<size_type>(v.size());
      }

      static void get_dimensions(matrix_type const & A,size_type & m, size_type & n)
      {
        m = static_cast<size_type>(A.rows());
        n = static_cast<size_type>(A.cols());
      }

      static void resize(vector_type & x,size_type n)
      {
        x.setZero(idx(n));
      }

      static void resize(idx_vector_type & x,size_type n)
      {
        x.setZero(idx(n));
      }

      /**
      * Resize and clear a sparse matrix, then reserve room to fill it element by element.
      *
      * The reserve() matters. Without it the matrix is left in compressed mode, where
      * inserting an element that is not already present has to shift the whole storage
      * array along -- turning Jacobian assembly into an O(nnz) operation per element.
      * reserve() switches the matrix to uncompressed mode with a slot budget per row,
      * which makes insertion amortised constant time.
      *
      * Twelve is the natural budget: a constraint row touches at most two bodies and
      * each body contributes six columns. Eigen grows a row that needs more, so this is
      * a hint and not a limit; it is simply the value that avoids reallocating for the
      * Jacobian, which is the matrix this path is on.
      *
      * The matrix stays uncompressed afterwards. That is fine -- Eigen's InnerIterator,
      * and hence every product below, handles both modes -- and saves a compression pass
      * that would be thrown away on the next assembly.
      */
      static void resize(matrix_type & A,size_type m,size_type n)
      {
        A.resize(idx(m), idx(n));
        A.reserve(Eigen::Matrix<eigen_index_type, Eigen::Dynamic, 1>::Constant(idx(m), 12));
      }

      /**
      *computes: y = prod(A,x) + b
      */
      static void prod(matrix_type const & A,vector_type const & x, vector_type const & b, vector_type & y)
      {
        y = A * x + b;
      }

      /**
      *computes: y = prod(A,x) - b
      */
      static void prod_minus(matrix_type const & A,vector_type const & x, vector_type const & b, vector_type & y)
      {
        y = A * x - b;
      }

      /**
      *computes: y = prod(A,x)
      */
      static void prod(matrix_type const & A,vector_type const & x, vector_type & y)
      {
        y = A * x;
      }

      /**
      *computes: y += prod(A,x)
      */
      static void prod_add(matrix_type const & A,vector_type const & x, vector_type & y)
      {
        ensure_size(y, static_cast<size_type>(A.rows()));
        y += A * x;
      }

      /**
      *computes: y = prod(A,x)*s
      */
      static void prod(matrix_type const & A,vector_type const & x, vector_type & y, real_type const & s)
      {
        y = (A * x) * s;
      }

      /**
      *computes: y += prod(A,x)*s
      */
      static void prod_add(matrix_type const & A,vector_type const & x, vector_type & y, real_type const & s)
      {
        ensure_size(y, static_cast<size_type>(A.rows()));
        y += (A * x) * s;
      }

      /**
      *computes: y = prod(trans(A),x)
      */
      static void prod_trans(matrix_type const & A,vector_type const & x, vector_type & y)
      {
        y = A.transpose() * x;
      }

      /**
      *computes: y = prod(trans(A),x) + b
      */
      static void prod_trans(matrix_type const & A,vector_type const & x, vector_type const & b, vector_type & y)
      {
        y = A.transpose() * x + b;
      }

      /**
      *computes: x *= s
      */
      static void prod(vector_type & x, real_type const & s )
      {
        x *= s;
      }

      /**
      *computes: x = -y
      */
      static void assign_minus(vector_type const & y,  vector_type & x)
      {
        x = -y;
      }

      /**
      *computes: A = J W J^T, the Schur complement the velocity-level solver works on.
      *
      * The uBLAS policy has to spell this out in three steps through sparse_prod, with a
      * comment about it failing on large matrices. Here it is one expression; Eigen sizes
      * and compresses the result itself.
      */
      static void compute_system_matrix(matrix_type const & invM, matrix_type const & J, system_matrix_type & A)
      {
        A.m_A = J * invM * matrix_type(J.transpose());

        A.m_d.setZero(A.m_A.rows());
        for(Eigen::Index i = 0; i < A.m_A.rows(); ++i)
          A.m_d(i) = A.m_A.coeff(i, i);
      }

      /**
      *computes: y = prod(A,x) + b, for the assembled system matrix.
      */
      static void prod(system_matrix_type const & A,vector_type const & x, vector_type const & b, vector_type & y)
      {
        y = A.m_A * x + b;
      }

    public:

      ///< Interface to support the GaussSeidel NCP solver.

      /**
      * Returns row i of A dotted with x.
      *
      * Written as an explicit walk over the row rather than A.row(i).dot(x) so that it
      * works whether or not the matrix has been compressed -- see resize() above, which
      * deliberately leaves it uncompressed.
      */
      static real_type row_prod(system_matrix_type const & A,size_type i,vector_type const & x)
      {
        assert(idx(i) < A.m_A.rows() || !"row_prod(): row index out of bounds");

        real_type sum = value_traits::zero();
        for(typename matrix_type::InnerIterator it(A.m_A, idx(i)); it; ++it)
          sum += it.value() * x(it.index());
        return sum;
      }

      /**
      * This method is specifically introduced to support the GaussSeidel NCP solver.
      */
      static void init_system_matrix(system_matrix_type & /*A*/, vector_type const & /*x*/)
      {
        // do nothing
      }

      /**
      * This method is specifically introduced to support the GaussSeidel NCP solver.
      */
      static void update_system_matrix(system_matrix_type & /*A*/, size_type /*i*/, real_type const & /*dx*/)
      {
        // do nothing...
      }

    };

  } // namespace mbd
} // namespace OpenTissue
// OPENTISSUE_DYNAMICS_MBD_MATH_MBD_EIGEN3_MATH_POLICY_H
#endif

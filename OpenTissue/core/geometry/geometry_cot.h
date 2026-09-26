#ifndef OPENTISSUE_CORE_GEOMETRY_GEOMETRY_COT_H
#define OPENTISSUE_CORE_GEOMETRY_GEOMETRY_COT_H
//
// OpenTissue Template Library
// - A generic toolbox for physics-based modeling and simulation.
// Copyright (C) 2008 Department of Computer Science, University of Copenhagen.
//
// OTTL is licensed under zlib: http://opensource.org/licenses/zlib-license.php
//
#include <OpenTissue/configuration.h>

#include <cmath>   // Needed for std::sqrt and std::fabs

namespace OpenTissue
{
  namespace geometry
  {

    /**
    * Cotangent of two vectors.
    * This implementation is based on the paper:
    *
    *    Meyer, M., Desbrun, M., Schröder, P., AND Barr, A. H. Discrete Differential Geometry Operators for Triangulated 2-Manifolds, 2002. VisMath.
    *
    * This metod computes the cotangent angle between two vectors (pi-p) and (pj-p).
    *
    * Let the angle between the two vectors \f$\vec u\f$ and \f$\vec v\f$ be \f$\theta\f$. Then by defnition
    *
    * \f[ \cot(\theta) = \frac{\cos \theta}{\sin \theta} \f]
    *
    * and we know that
    *
    * \f[ \cos(\theta) =  \frac{ u \cdot v  }{ \|u\| \|v\| } \f]
    *
    * and
    *
    * \f[ \|u \times v\| = \|u\|\|v\| \sin \theta \f]
    *
    * since \f$1 = \cos^2 \theta + \sin^2 \theta\f$ we re-write
    *
    * \f[ \|u\|\|v\| \sin \theta  = \|u\|\|v\|\left( \pm \sqrt{1 - \cos^2 \theta }\right) \f]
    *
    * Knowing that the angle between two vectors is between 0 and \f$\pi\f$, we can throw away
    * the negative solution of \f$\sin\theta\f$. Substituting the expression for \f$\cos \theta\f$ and
    * isolating \f$\sin \theta\f$ we get
    *
    * \f[ \sin \theta  = \frac{  \sqrt{\|u\|^2\|v\|^2 -  (u \cdot v )^2 } }{\|u\|\|v\|} \f]
    *
    * Finally substituting the expressions for cos and sin into the equation for cot we get
    *
    * \f[ \cot(\theta)  = \frac{u \cdot v}{ \sqrt{\|u\|^2\|v\|^2 -  (u \cdot v )^2 } } \f]
    *
    * This is the formula we use for computing cot.
    *
    * @param p      The common tail point of the two vectors
    * @param pi     The head point of one vector.
    * @param pj     The head point of the other vector.
    *
    * @return       The computed value.
    */
    template<typename vector_type>
    typename vector_type::value_type cot(vector_type const & p, vector_type const & pi, vector_type const & pj)  
    {
      using std::sqrt;
      using std::fabs;

      typedef typename vector_type::value_type   real_type;

      static const real_type zero = real_type(); // By standard default constructed integral types are zero

      vector_type u = pi-p;
      vector_type v = pj-p;
      real_type dot = u*v;
      real_type denom = sqrt( (u*u)*(v*v) - dot*dot );

      return (fabs(denom)>zero)? (dot/denom) : zero; 
    }

  } // namespace geometry
} // namespace OpenTissue

//OPENTISSUE_CORE_GEOMETRY_GEOMETRY_COT_H
#endif

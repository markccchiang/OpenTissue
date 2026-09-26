#ifndef OPENTISSUE_CORE_MATH_MATH_KMEANS_H
#define OPENTISSUE_CORE_MATH_MATH_KMEANS_H
//
// OpenTissue Template Library
// - A generic toolbox for physics-based modeling and simulation.
// Copyright (C) 2008 Department of Computer Science, University of Copenhagen.
//
// OTTL is licensed under zlib: http://opensource.org/licenses/zlib-license.php
//
#include <OpenTissue/configuration.h>

#include <OpenTissue/core/math/math_random.h>
#include <OpenTissue/core/math/math_matrix3x3.h>
#include <OpenTissue/core/math/math_covariance.h>
#include <OpenTissue/core/math/math_constants.h>
#include <OpenTissue/core/math/math_value_traits.h>

#include <boost/iterator/indirect_iterator.hpp>

#include <algorithm>
#include <list>
#include <vector>
#include <cassert>

namespace OpenTissue
{
  namespace math
  {
    namespace detail
    {
      /**
      * A General Purpose KMeans algorithm Implementation.
      *
      * The class takes two template arguments: V a vector
      * type and M a matrix type.
      */
      template< typename V, typename M >
      class KMeans
      {
      protected:

        typedef V                                vector_type;
        typedef typename V::value_type           real_type;
        typedef M                                matrix_type;

        typedef OpenTissue::math::ValueTraits<real_type>  value_traits;

        typedef std::list< vector_type * >                                   feature_ptr_container;
        typedef typename feature_ptr_container::iterator                     feature_ptr_iterator;
        typedef boost::indirect_iterator<feature_ptr_iterator,vector_type>   feature_iterator;

      protected:

        /**
         * A Cluster Class.
         * This class is used to store information about a
         * given cluster, such as the mean point and co-variance
         * of the cluster.
         */
        class cluster_type
        {
        public:

          vector_type            m_mean;     ///< The mean point, i.e. the cluster center.
          matrix_type            m_C;        ///< Covariance matrix.
          matrix_type            m_invC;     ///< The inverse covariance matrix.
          size_t                 m_index;    ///< A unique cluster index
          feature_ptr_container  m_features; ///< Pointers to all features in this cluster.

        public:

          feature_iterator begin()  { return m_features.begin(); }
          feature_iterator end()    { return m_features.end();   }

        public:

          cluster_type()
            : m_mean(value_traits::zero(),value_traits::zero(),value_traits::zero())
            , m_C( value_traits::one(), value_traits::zero(), value_traits::zero(), value_traits::zero(), value_traits::one(), value_traits::zero(), value_traits::zero(), value_traits::zero(), value_traits::one()  )
            , m_invC( value_traits::one(), value_traits::zero(), value_traits::zero(), value_traits::zero(), value_traits::one(), value_traits::zero() , value_traits::zero(), value_traits::zero(), value_traits::one() )
            , m_index(0u)
          {}

          void update()
          {
            using OpenTissue::math::covariance;
            using OpenTissue::math::inverse;

            covariance( begin(), end(), m_mean, m_C);
            m_invC = inverse(m_C);
          }

        };


        /**
         * Feature Point Cluster Membership Information.
         * This class encapsulates information about feature
         * points such as indexes and the cluster they belong to.
         */
        class membership_info
        {
        public:

          size_t         m_index;    ///< The index of the original feature point.
          cluster_type * m_cluster;  ///< A pointer to the cluster that the feature point belongs to.
          vector_type  * m_p;        ///< A pointer to the feature vector.

        public:

          membership_info()
            : m_index(0)
            , m_cluster(0)
            , m_p(0)
          {}

          membership_info(size_t const & idx, vector_type * p)
            : m_index(idx)
            , m_cluster(0)
            , m_p(p)
          {}

          membership_info(membership_info const & info)
            : m_index(info.m_index)
            , m_cluster(info.m_cluster)
            , m_p(info.m_p)
          {}

        };


        typedef typename std::vector< membership_info >          membership_container;
        typedef typename std::list<cluster_type>                 cluster_container;

        cluster_container      m_clusters;           ///< Clusters.
        membership_container   m_memberships;        ///< Feature to cluster membership container.

      public:

        typedef typename cluster_container::iterator             cluster_iterator;
        typedef typename membership_container::iterator          membership_iterator;

        /**
        * Cluster Begin Iterator.
        *
        * @return   An iterator to the first cluster.
        */
        cluster_iterator cluster_begin()  { return m_clusters.begin(); }

        /**
        * Cluster End Iterator.
        *
        * @return   An iterator to one past the last cluster.
        */
        cluster_iterator cluster_end()    { return m_clusters.end();   }

        /**
        * Membership Begin Iterator.
        *
        * @return    An iterator to the first feature point membership information.
        */
        membership_iterator membership_begin() { return m_memberships.begin(); }

        /**
        * Membership End Iterator.
        *
        * @return    An iterator to one past the last feature point membership information.
        */
        membership_iterator membership_end()   { return m_memberships.end();   }

        /**
        * Get number of feature points.
        *
        * @return    The number of feature points.
        */
        size_t feature_size() const { return m_memberships.size(); }

      protected:

        static real_type squared_distance( vector_type const & a, vector_type const & b )
        {
          using OpenTissue::math::inner_prod;
          vector_type const diff = a - b;
          return inner_prod( diff, diff );
        }

        /**
        * Pick one of the indices 0..N-1 with equal probability.
        */
        static size_t pick_uniformly( OpenTissue::math::Random<real_type> & uniform, size_t const & N )
        {
          size_t const pick = static_cast<size_t>( uniform() * N );
          return pick < N ? pick : N - 1u;
        }

        /**
        * Append a cluster, centered at the given point.
        */
        void add_cluster( vector_type const & center )
        {
          m_clusters.push_back( cluster_type() );
          m_clusters.back().m_mean  = center;
          m_clusters.back().m_index = m_clusters.size() - 1u;
        }

        /**
        * Initialize KMeans Algorithm.
        * This method creates an initial set of clusters and assign feature points to them.
        *
        * @param begin    An iterator to the first feature point.
        * @param end      An iterator to one past the last feature point.
        * @param K        The wanted number of clusters.
        */
        template<typename vector_iterator>
        void initialize(
          vector_iterator const & begin
          , vector_iterator const & end
          , size_t const & K
          )
        {
          using std::min;

          // Assign all feature points to the zero-indexed cluster

          size_t N = std::distance( begin, end );
          assert(N > K || !"KMeans::initialize() There must be more feature points than number of clusters.");

          {
            m_memberships.resize(N);
            size_t          index = 0u;
            vector_iterator p     = begin;
            for( ; p != end; ++p, ++index)
              m_memberships[index] = membership_info( index, &(*p) );
          }

          // Choose the initial cluster centers by k-means++ seeding (Arthur and Vassilvitskii,
          // 2007): the first center is a feature point picked at random, and each further one
          // is a feature point picked with probability proportional to its squared distance
          // to the nearest center chosen so far. Centers then start spread over the data.
          //
          // The centers used to be placed uniformly at random in the bounding box of the
          // feature points. A center could land where there were no points, and its cluster
          // stayed empty for good, while others merged separate groups of points.
          OpenTissue::math::Random<real_type> uniform;   // uniform on [0,1)

          m_clusters.clear();
          add_cluster( *(m_memberships[ pick_uniformly( uniform, N ) ].m_p) );

          // Squared distance from every feature point to its nearest center so far.
          std::vector<real_type> squared_distances( N );
          for(size_t n = 0; n < N; ++n)
            squared_distances[n] = squared_distance( *(m_memberships[n].m_p), m_clusters.back().m_mean );

          while( m_clusters.size() < K )
          {
            real_type total = value_traits::zero();
            for(size_t n = 0; n < N; ++n)
              total += squared_distances[n];

            size_t pick = 0u;
            if( total > value_traits::zero() )
            {
              real_type const threshold = uniform() * total;
              real_type       sum       = value_traits::zero();
              pick = N - 1u;  // in case round-off keeps the running sum below the threshold
              for(size_t n = 0; n < N; ++n)
              {
                sum += squared_distances[n];
                if( sum > threshold )
                {
                  pick = n;
                  break;
                }
              }
            }
            else
            {
              // Every feature point coincides with a center already; any point will do.
              pick = pick_uniformly( uniform, N );
            }

            add_cluster( *(m_memberships[pick].m_p) );
            for(size_t n = 0; n < N; ++n)
              squared_distances[n] = min( squared_distances[n], squared_distance( *(m_memberships[n].m_p), m_clusters.back().m_mean ) );
          }

          // Finally we assign the membership of the feature points to the initial clusters
          distribute_features();
        }

        /**
        * This method re-assigns feature points to clusters. Afterwards the method
        * tries to re-estimate the clusters using a covariance analysis of the
        * assigned feature points.
        *
        * @return     If a change occured in the reassignment of feature
        *             points then the return value is true. If the return
        *             value is false then it simply means that the clusters
        *             did not change. In other words we would have converged.
        */
        bool const distribute_features( )
        {
          using OpenTissue::math::inner_prod;

          // Clear all the assignment of feature points to all the clusters.
          {
            cluster_iterator end     = m_clusters.end();
            cluster_iterator cluster = m_clusters.begin();
            for(;cluster!=end;++cluster)
              cluster->m_features.clear();
          }

          bool changed = false;
          // Re-assign feature points to the closest cluster and check for changes in the assignment.
          {
            membership_iterator m     = m_memberships.begin();
            membership_iterator m_end = m_memberships.end();
            for(;m != m_end; ++m)
            {
              vector_type  const * p     = m->m_p;
              cluster_type       * owner = 0;

              real_type min_squared_distance = value_traits::infinity();
              cluster_iterator c     = m_clusters.begin();
              cluster_iterator c_end = m_clusters.end();
              for(;c!=c_end;++c)
              {
                vector_type diff = *p - c->m_mean;
                // Mahalonobis type of distance measure, seems to make convergence really bad!!!
                //   real_type squared_distance = inner_prod( diff,  prod( c->m_invC , diff) );
                // So we use Euclidean ("spherical") distances
                real_type squared_distance = inner_prod( diff, diff );
                if(squared_distance < min_squared_distance)
                {
                  min_squared_distance = squared_distance;
                  owner = &(*c);
                }
              }
              assert(owner || !"distribute_features() Fatal error could not find a cluster owner for feature point");
              changed = (m->m_cluster != owner) ? true : changed;
              m->m_cluster = owner;
              owner->m_features.push_back( const_cast<vector_type*>(  p  )  );
            }
          }

          // Update cluster information based on new re-assigned feature points.
          for(cluster_iterator cluster=m_clusters.begin();cluster!=m_clusters.end();++cluster)
          {
            if(! cluster->m_features.empty())
              cluster->update();
          }

          // A cluster that got no feature points would otherwise never move again. Move its
          // center onto the feature point that is farthest from its own cluster's center;
          // the next re-assignment then gives it that point. Each move lowers the sum of
          // squared distances, so this cannot make the iteration cycle.
          for(cluster_iterator cluster=m_clusters.begin();cluster!=m_clusters.end();++cluster)
          {
            if(! cluster->m_features.empty())
              continue;

            membership_info * farthest     = 0;
            real_type         max_distance = -value_traits::one();
            for(membership_iterator m = m_memberships.begin(); m != m_memberships.end(); ++m)
            {
              if( m->m_cluster->m_features.size() < 2u )  // do not empty another cluster
                continue;
              real_type const d = squared_distance( *(m->m_p), m->m_cluster->m_mean );
              if( d > max_distance )
              {
                max_distance = d;
                farthest     = &(*m);
              }
            }
            if(!farthest)
              break;  // no cluster can spare a feature point

            cluster->m_mean = *(farthest->m_p);
            changed = true;
          }
          return changed;
        }

        /**
        * The sum of squared distances from every feature point to the center of its cluster.
        * K-means lowers it every iteration; of several runs, the one with the smallest value
        * found the best clustering.
        */
        real_type sum_of_squared_distances() const
        {
          real_type sum = value_traits::zero();
          for(typename membership_container::const_iterator m = m_memberships.begin(); m != m_memberships.end(); ++m)
            sum += squared_distance( *(m->m_p), m->m_cluster->m_mean );
          return sum;
        }

        /**
        * Run K-means once from a k-means++ initialization.
        */
        template<typename vector_iterator>
        void run_once(
          vector_iterator const & begin
          , vector_iterator const & end
          , size_t const & K
          , size_t & iteration
          , size_t const & max_iterations
          )
        {
          iteration = 0u;
          initialize(begin,end,K);
          bool changed = false;
          do
          {
            ++iteration;
            changed = distribute_features();
            if(iteration >= max_iterations)
              return;
          }while(changed);
        }

      public:

        /**
        * K-means.
        *
        * K-means converges to a local minimum, which depends on the initial centers. Even
        * with k-means++ seeding a run occasionally starts two centers in one group of
        * points and merges two other groups, so the algorithm is run several times and the
        * best clustering is kept.
        *
        * @param begin            An iterator to the first feature point.
        * @param end              An iterator to the position one past the last feature point.
        * @param K                The number of clusters
        * @param iteration        Upon return this argument holds the number of iterations
        *                         done by the run that was kept.
        * @param max_iterations   The maximum number of iterations to perform the KMeans algorithm.
        * @param restarts         The number of times to run the algorithm. At least one run is made.
        *
        */
        template<typename vector_iterator>
        void run( 
          vector_iterator const & begin
          , vector_iterator const & end
          , size_t const & K
          , size_t & iteration
          , size_t const & max_iterations
          , size_t const & restarts = 10u
          )
        {
          std::vector<vector_type> best_centers;
          real_type                best_sum       = value_traits::infinity();
          size_t                   best_iteration = 0u;

          size_t const runs = restarts > 0u ? restarts : 1u;
          for(size_t r = 0u; r < runs; ++r)
          {
            run_once(begin, end, K, iteration, max_iterations);

            real_type const sum = sum_of_squared_distances();
            if( sum < best_sum || best_centers.empty() )
            {
              best_sum       = sum;
              best_iteration = iteration;
              best_centers.clear();
              for(cluster_iterator c = m_clusters.begin(); c != m_clusters.end(); ++c)
                best_centers.push_back( c->m_mean );
            }
          }

          // Restore the best run. Its centers are the means of their clusters, so assigning
          // every point to its nearest center again reproduces that run's clustering.
          size_t i = 0u;
          for(cluster_iterator c = m_clusters.begin(); c != m_clusters.end(); ++c, ++i)
            c->m_mean = best_centers[i];
          distribute_features();
          iteration = best_iteration;
        }
      };

    } // namespace detail


    /**
    * K-Means Algorithm.
    *
    * @param begin            An iterator to the first feature point.
    * @param end              An iterator to the position one past the last feature point.
    * @param centers      Upon return this container holds the cluster centers that have been found.
    * @param membership   Upon return this container holds cluster membership information of
    *                     the feature points. That is the i'th feature points p[i] belongs
    *                     to cluster c[m[i]].
    *
    *                          p[i] \in c[m[i]]
    *
    * @param K                The number of clusters
    * @param iteration        Upon return this argument holds the number of iterations that was done.
    * @param max_iterations   The maximum number of iterations to perform the KMeans algorithm. Usually a value of 50 works okay.
    * @param restarts         The number of times to run the algorithm, keeping the best clustering.
    *                         K-means finds a local minimum that depends on where it starts; more
    *                         runs make a poor one less likely, at a proportional cost.
    *
    */
    template<typename vector_iterator, typename vector_container, typename index_container>
    inline void kmeans(
      vector_iterator const & begin
      , vector_iterator const & end
      , vector_container & centers
      , index_container & membership
      , size_t K
      , size_t & iteration
      , size_t const & max_iterations
      , size_t const & restarts = 10u
      )
    {
      typedef typename vector_container::value_type  vector_type;
      typedef typename vector_type::value_type       real_type;
      typedef OpenTissue::math::Matrix3x3<real_type> matrix_type;

      typedef OpenTissue::math::detail::KMeans<vector_type, matrix_type> kmeans_algorithm;

      typedef typename kmeans_algorithm::cluster_iterator    cluster_iterator;
      typedef typename kmeans_algorithm::membership_iterator membership_iterator;

      kmeans_algorithm kmeans;

      kmeans.run( begin , end, K, iteration, max_iterations, restarts );

      centers.clear();
      centers.resize(K);

      cluster_iterator c_end = kmeans.cluster_end();
      cluster_iterator c     = kmeans.cluster_begin();
      for(; c!=c_end; ++c)
        centers[c->m_index] = c->m_mean;

      membership.clear();
      membership.resize( kmeans.feature_size() );

      membership_iterator m_end = kmeans.membership_end();
      membership_iterator m     = kmeans.membership_begin();
      for(;m!=m_end;++m)
        membership[m->m_index] = m->m_cluster->m_index;
    }

  } // namespace math
} // namespace OpenTissue

// OPENTISSUE_CORE_MATH_MATH_KMEANS_H
#endif

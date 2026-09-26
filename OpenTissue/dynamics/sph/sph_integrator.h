#ifndef OPENTISSUE_DYNAMICS_SPH_SPH_INTEGRATOR_H
#define OPENTISSUE_DYNAMICS_SPH_SPH_INTEGRATOR_H
//
// OpenTissue Template Library
// - A generic toolbox for physics-based modeling and simulation.
// Copyright (C) 2008 Department of Computer Science, University of Copenhagen.
//
// OTTL is licensed under zlib: http://opensource.org/licenses/zlib-license.php
//
#include <OpenTissue/configuration.h>

namespace OpenTissue
{
  namespace sph
  {

    /**
    * SPH Compute Base Class.
    */
    template< typename Types >
    class Integrator
    {
    public:
      typedef typename Types::real_type  real_type;
      typedef typename Types::vector  vector;
      typedef typename Types::particle  particle;
      typedef typename Types::particle_container  particle_container;
      typedef typename Types::collision_detection  collision_detection;

    public:
      /**
      * Default Constructor.
      */
      Integrator(const real_type& timestep = 0.01, const real_type& restitution = 0.)
        : m_dt(timestep)
        , m_dt2(timestep*timestep)
        , m_invdt(1./timestep)
        , m_invdt2(1./(timestep*timestep))
        , m_r(restitution)
        , m_colisys(NULL)
      {
      }

      /**
      * Deconstructor.
      */
      virtual ~Integrator()
      {
      }

    public:
      /**
      * Integrate (mandatory)
      *
      * @param colisys   The collision detection system.
      */
      void setCollisionSystem(collision_detection* colisys)
      {
        m_colisys = colisys;
      }

      /**
      * Initialize particles (optional)
      *
      * @param begin   An iterator to the first particle to undergo initialization.
      * @param end     An iterator to one past the last particle.
      */
      void initialize_particles(typename particle_container::iterator begin, typename particle_container::iterator end) const
      {
        for (typename particle_container::iterator par = begin; par != end; ++par)
          initialize(*par);
      }

      /**
      * Integrate particles (mandatory)
      *
      * @param begin   An iterator to the first particle to undergo numerical integration.
      * @param end     An iterator to one past the last particle.
      */
      void integrate_particles(typename particle_container::iterator begin, typename particle_container::iterator end) const
      {
        for (typename particle_container::iterator par = begin; par != end; ++par) {
          particle& p = *par;
          if (!p.fixed())
            integrate(p);
        }
      }

      virtual void initialize(particle& /*par*/) const {}

      virtual void integrate(particle& par) const = 0;

    protected:
      real_type  m_dt;   /// timestep
      real_type  m_dt2;   /// timestep squared
      real_type  m_invdt;   /// inverse timestep
      real_type  m_invdt2;   /// inverse timestep squared
      real_type  m_r;    /// restitution

      collision_detection*  m_colisys;

    }; // End class Integrator

  } // namespace sph
} // namespace OpenTissue

// OPENTISSUE_DYNAMICS_SPH_SPH_INTEGRATOR_H
#endif

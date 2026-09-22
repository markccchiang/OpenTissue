#ifndef OPENTISSUE_UTILITY_UTILITY_QHULL_H
#define OPENTISSUE_UTILITY_UTILITY_QHULL_H
//
// OpenTissue Template Library
// - A generic toolbox for physics-based modeling and simulation.
// Copyright (C) 2008 Department of Computer Science, University of Copenhagen.
//
// OTTL is licensed under zlib: http://opensource.org/licenses/zlib-license.php
//
#include <OpenTissue/configuration.h>

  //////////////////////////////////////////////////////////////////
  //
  // This pulls in Qhull's *reentrant* C library, libqhull_r, rather than the
  // original libqhull.
  //
  // The two differ in where Qhull keeps its state. libqhull keeps it in
  // globals, reached through a "qh" macro; libqhull_r keeps it in a qhT that
  // the caller owns and passes to every function. Upstream deprecated the
  // former, and packagers have followed -- vcpkg builds only the reentrant
  // library -- so the non-reentrant one is simply not available everywhere
  // any more.
  //
  // The headers are still included through extern "C". Qhull advises including
  // qhull_ra.h instead of the individual headers, but that pulls in math.h in a
  // way MSVC objects to from inside an extern "C" block.
  //
#if defined(__cplusplus)
  extern "C"
  {
#endif
#include <stdio.h>
#include <stdlib.h>
#include <libqhull_r/libqhull_r.h>
#include <libqhull_r/mem_r.h>
#include <libqhull_r/qset_r.h>
#include <libqhull_r/geom_r.h>
#include <libqhull_r/merge_r.h>
#include <libqhull_r/poly_r.h>
#include <libqhull_r/io_r.h>
#include <libqhull_r/stat_r.h>
#if defined(__cplusplus)
  }
#endif

//OPENTISSUE_UTILITY_UTILITY_QHULL_H
#endif

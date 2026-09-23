add_subdirectory(benchmark_bfgs)
add_subdirectory(benchmark_gjk)
add_subdirectory(benchmark_svd)
add_subdirectory(dynamic_table_dispatcher)

# These two compare OpenTissue's uBLAS-based solvers with Eigen on the same input, so they
# need Eigen. Skip them rather than fail the configure when it is absent.
if(OPENTISSUE_HAVE_EIGEN3)
  add_subdirectory(benchmark_lu)
  add_subdirectory(benchmark_swe)
else()
  message(STATUS "Skipping demos 'benchmark_lu' and 'benchmark_swe' (needs EIGEN3)")
endif()

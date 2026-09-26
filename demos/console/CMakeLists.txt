add_subdirectory(benchmark_bfgs)
add_subdirectory(benchmark_gjk)
add_subdirectory(benchmark_svd)
add_subdirectory(dynamic_table_dispatcher)
add_subdirectory(paraview_export)
add_subdirectory(paraview_shallow_water)
add_subdirectory(paraview_cantilever)

# These two compare OpenTissue's uBLAS-based solvers with Eigen on the same input. They are
# the record of why OpenTissue does not use Eigen, and the only code in the tree that does:
# the library itself has no Eigen dependency, so it is looked for here and nowhere else.
# Skip them rather than fail the configure when it is absent.
find_package(Eigen3 QUIET NO_MODULE)
if(TARGET Eigen3::Eigen)
  add_subdirectory(benchmark_lu)
  add_subdirectory(benchmark_swe)
else()
  message(STATUS "Skipping demos 'benchmark_lu' and 'benchmark_swe' (need Eigen)")
endif()

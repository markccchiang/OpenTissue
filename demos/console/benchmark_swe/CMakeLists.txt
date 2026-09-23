add_executable(benchmark_swe src/benchmark_swe.cpp)

target_link_libraries(benchmark_swe
  PRIVATE
    OpenTissue
    Eigen3::Eigen
)

install(
  TARGETS benchmark_swe
  RUNTIME DESTINATION  bin/units
  COMPONENT Demos
  )

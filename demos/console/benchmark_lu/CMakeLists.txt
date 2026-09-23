add_executable(benchmark_lu src/benchmark_lu.cpp)

target_link_libraries(benchmark_lu
  PRIVATE
    OpenTissue
    Eigen3::Eigen
)

install(
  TARGETS benchmark_lu
  RUNTIME DESTINATION  bin/units
  COMPONENT Demos
  )

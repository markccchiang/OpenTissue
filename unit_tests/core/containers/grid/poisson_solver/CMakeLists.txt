add_executable(unit_poisson_solver src/unit_poisson_solver.cpp)

target_link_libraries(unit_poisson_solver
  PRIVATE
    Boost::unit_test_framework
    OpenTissue
)

ot_add_test(unit_poisson_solver)

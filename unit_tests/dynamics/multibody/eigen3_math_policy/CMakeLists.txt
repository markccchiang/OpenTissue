#----------------------------------------------------------------------
#
# The Eigen math policy for mbd. Only built when Eigen was found -- the
# parent adds this directory through ot_add_test_subdirectory(... EIGEN3).
#
# Eigen3::Eigen is linked here rather than on the OpenTissue interface
# target on purpose: it carries INTERFACE_COMPILE_FEATURES "cxx_std_14",
# and OpenTissue's headers are meant to stay usable from C++11.
#
add_executable(unit_eigen3_math_policy src/unit_eigen3_math_policy.cpp)

target_link_libraries(unit_eigen3_math_policy
  PRIVATE
    Boost::unit_test_framework
    OpenTissue
    Eigen3::Eigen
)

ot_add_test(unit_eigen3_math_policy)

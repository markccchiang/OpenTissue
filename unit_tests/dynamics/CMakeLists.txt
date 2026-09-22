ot_add_test_subdirectory( multibody QHULL )

# The Eigen math policy test lives inside multibody/ but, unlike the rest of that directory,
# needs nothing from Qhull. Adding it from here rather than from multibody/CMakeLists.txt
# keeps it out of that QHULL gate -- otherwise it is silently skipped wherever Qhull is
# missing, which is exactly what happened on the Windows CI runner, leaving the policy
# untested on the one compiler most likely to object to it.
ot_add_test_subdirectory( multibody/eigen3_math_policy EIGEN3 )

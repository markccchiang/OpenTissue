add_executable(unit_convex_hull src/unit_convex_hull.cpp)

target_link_libraries(unit_convex_hull
  PRIVATE
    Qhull::qhull_r
    Boost::unit_test_framework
    OpenTissue
)

ot_add_test(unit_convex_hull)

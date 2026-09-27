add_executable(unit_laplacian_blur src/unit_laplacian_blur.cpp)

target_link_libraries(unit_laplacian_blur
  PRIVATE
    Boost::unit_test_framework
    OpenTissue
)

ot_add_test(unit_laplacian_blur)

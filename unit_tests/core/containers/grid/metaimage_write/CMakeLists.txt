add_executable(unit_metaimage_write src/unit_metaimage_write.cpp)

target_link_libraries(unit_metaimage_write
  PRIVATE
    Boost::unit_test_framework
    OpenTissue
)

ot_add_test(unit_metaimage_write)

add_executable(unit_identifier src/unit_identifier.cpp)

target_link_libraries(unit_identifier
  PRIVATE
    Boost::unit_test_framework
    OpenTissue
)

ot_add_test(unit_identifier)

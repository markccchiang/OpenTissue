add_executable(unit_prime_numbers src/unit_prime_numbers.cpp)

target_link_libraries(unit_prime_numbers
  PRIVATE
    Boost::unit_test_framework
    OpenTissue
)

ot_add_test(unit_prime_numbers)

# prime_search() used to scan towards INT_MAX for sizes above ~48000 and never return. Should
# that come back, fail after a minute instead of holding the CI job until its own timeout.
set_tests_properties(unit_prime_numbers PROPERTIES TIMEOUT 60)

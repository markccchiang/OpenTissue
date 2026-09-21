#-----------------------------------------------------------------------------
#
# The PNG round-trip test needs a real PNG to read.
#
# This used to be downloaded from Wikimedia at configure time, which made both
# the build and the test depend on a third-party website staying up and serving
# that exact thumbnail URL. It stopped doing so -- the URL now answers 400 --
# and because the download status was never checked, the result was an empty
# file and a test that failed for reasons having nothing to do with OpenTissue.
#
# The fixture is committed instead: a 2 KB 32x32 RGB image, small enough to live
# in the repository and enough for a read/write round trip.
#
set(PNG_TEST_FILE "${CMAKE_CURRENT_BINARY_DIR}/pax.png")

# The test reads "pax.png" from its working directory, which is the binary dir.
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/data/test_image.png"
  "${PNG_TEST_FILE}"
  COPYONLY
)

add_executable(unit_png src/unit_png.cpp)

target_link_libraries(unit_png
  PRIVATE
    Boost::unit_test_framework
    OpenTissue
    PNG::PNG
)

ot_add_test(unit_png)

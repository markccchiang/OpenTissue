
set(GRAPHICS_GLFW_SOURCES
  glfw/glfw_window.cpp
  glfw/glfw_application.cpp
)

add_library(OpenTissueGraphics
  ${GRAPHICS_GLFW_SOURCES}
)

# The public headers of this library include <GLFW/glfw3.h> and the GL headers, so the
# graphics stack has to propagate to consumers -- PRIVATE headers alone left demos without
# the include paths they need.
target_link_libraries(OpenTissueGraphics
  PRIVATE
    headers
  PUBLIC
    OpenGL::GL
    GLEW::GLEW
    glfw
)

#-------------------------------------------------------------------------------
#
# Generate export headers
#
include(GenerateExportHeader)
generate_export_header(OpenTissueGraphics
  BASE_NAME graphics
  EXPORT_MACRO_NAME GRAPHICS_API
  DEPRECATED_MACRO_NAME GRAPHICS_DEPRECATED_API
  PREFIX_NAME OT_
  ${NO_BUILD_DEPRECATED}
)


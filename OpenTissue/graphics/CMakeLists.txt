
set(GRAPHICS_GLFW_SOURCES
  glfw/glfw_window.cpp
  glfw/glfw_application.cpp
)

add_library(OpenTissueGraphics
  ${GRAPHICS_GLFW_SOURCES}
)

# All of this is PUBLIC: the library's own headers include <GLFW/glfw3.h>, the GL headers
# and OpenTissue headers, so everything has to propagate to consumers. 'headers' used to be
# PRIVATE, which left anything linking this target without the include paths it needs.
target_link_libraries(OpenTissueGraphics
  PUBLIC
    headers
    OpenGL::GL
    GLEW::GLEW
    glfw
)

# Exported as OpenTissue::graphics, to match OpenTissue::headers.
set_target_properties(OpenTissueGraphics PROPERTIES EXPORT_NAME graphics)

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

# The generated header is included as <OpenTissue/graphics/graphics_export.h>, so it has to
# land beside the hand-written graphics headers in the install tree. The install(DIRECTORY)
# rule at the top level only copies the source tree, which this file is not part of.
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/graphics_export.h"
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/OpenTissue/graphics
)


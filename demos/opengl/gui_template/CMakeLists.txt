add_executable(gui_template src/application.cpp)

target_link_libraries(gui_template
  PRIVATE
    OpenTissue
    OpenTissueGraphics
)

# Pulls in graphics/core/main.h, which provides main() and calls createApplication().
target_compile_definitions(gui_template
  PRIVATE
    OT_INJECT_MAIN
)

install(
  TARGETS gui_template
  RUNTIME DESTINATION bin/demos
  COMPONENT Demos
  )

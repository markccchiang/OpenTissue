add_executable(paraview_cantilever src/paraview_cantilever.cpp)

target_link_libraries(paraview_cantilever
  PRIVATE
    OpenTissue
)

# The ParaView script that renders this demo's output, placed next to the program so that
# running both from the build directory needs no paths.
configure_file(render.py "${CMAKE_CURRENT_BINARY_DIR}/render.py" COPYONLY)

install(
  TARGETS paraview_cantilever
  RUNTIME DESTINATION  bin/units
  COMPONENT Demos
  )

add_executable(paraview_shallow_water src/paraview_shallow_water.cpp)

target_link_libraries(paraview_shallow_water
  PRIVATE
    OpenTissue
)

# The ParaView script that renders this demo's output, placed next to the program so that
# running both from the build directory needs no paths.
configure_file(render.py "${CMAKE_CURRENT_BINARY_DIR}/render.py" COPYONLY)

install(
  TARGETS paraview_shallow_water
  RUNTIME DESTINATION  bin/units
  COMPONENT Demos
  )

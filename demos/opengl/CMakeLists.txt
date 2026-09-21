#-----------------------------------------------------------------------------
#
# The character animation demo loads its skeleton, animations and skin from XML
# and its textures from PNG, so it needs both TinyXML and libpng. Skip it rather
# than fail the whole configure when either is unavailable.
#
if(OPENTISSUE_HAVE_TINYXML AND OPENTISSUE_HAVE_PNG)
  add_subdirectory(character_animation)
else()
  message(STATUS "Skipping demo 'character_animation' (needs TinyXML and libpng)")
endif()

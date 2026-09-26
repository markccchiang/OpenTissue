#------------------------------------------------------------------
#
# The Doxygen API documentation: `cmake --build <build> --target apidoc`, written to
# <build>/documentation/html/index.html.
#
# Only built on request. It is not part of the default build, since it regenerates pages for
# every header each time.
#
find_package(Doxygen OPTIONAL_COMPONENTS dot)

if(NOT Doxygen_FOUND)
  # Without this the option silently does nothing, and the only hint is a later
  # "No rule to make target apidoc".
  message(WARNING
    "OPENTISSUE_ENABLE_DOCUMENTATION is ON but Doxygen was not found, so there is no apidoc "
    "target. Install Doxygen (brew install doxygen, apt install doxygen) and Graphviz for the "
    "diagrams, then configure again.")
  return()
endif()

set(DOXYGEN_PROJECT_NAME   "OpenTissue")
set(DOXYGEN_PROJECT_NUMBER "${PROJECT_VERSION}")
set(DOXYGEN_PROJECT_BRIEF  "A generic toolbox for physics-based modeling and simulation")

# Show paths relative to the checkout, not the machine that built the pages, and #include lines
# the way a user writes them: <OpenTissue/...>.
set(DOXYGEN_STRIP_FROM_PATH     "${PROJECT_SOURCE_DIR}")
set(DOXYGEN_STRIP_FROM_INC_PATH "${PROJECT_SOURCE_DIR}")

# Document every header, not only those with a \file comment -- which is almost none of them,
# so without this the file list held two entries out of nine hundred.
set(DOXYGEN_EXTRACT_ALL YES)

# Render \f$ ... \f$ formulas in the browser. The default renders them with LaTeX, and where
# LaTeX is not installed every formula comes out as a broken image.
set(DOXYGEN_USE_MATHJAX YES)

# LaTeX macros that the formulas in the headers use but LaTeX does not define: \norm{x} and
# \mat A. Without them MathJax renders those formulas as errors.
set(DOXYGEN_FORMULA_MACROFILE "${CMAKE_CURRENT_SOURCE_DIR}/formula_macros.tex")

# The README is the front page, and the guides are pages of the site. INSTALL.md and
# CHANGELOG.md are included because the README links to them.
set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "${PROJECT_SOURCE_DIR}/README.md")

doxygen_add_docs(
  apidoc
  "${PROJECT_SOURCE_DIR}/README.md"
  "${PROJECT_SOURCE_DIR}/INSTALL.md"
  "${PROJECT_SOURCE_DIR}/CHANGELOG.md"
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${PROJECT_SOURCE_DIR}/OpenTissue"
  COMMENT "Generating the OpenTissue API documentation"
)

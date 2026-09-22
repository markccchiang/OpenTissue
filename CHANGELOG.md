# Changelog

All notable changes to OpenTissue are recorded here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

Anything older than the entry below predates this file; see the git history.

## [Unreleased]

A modernisation pass. The project had not built out of the box for some years: its
dependencies came from a private package remote that no longer existed, and roughly a
quarter of its headers no longer compiled against current standard libraries.

The version in `CMakeLists.txt` is still `0.994` and has deliberately not been bumped —
these changes are breaking enough to deserve a deliberate version decision rather than an
incidental one.

### Added

- **Continuous integration** (`.github/workflows/ci.yml`) covering Linux (GCC and Clang),
  macOS (AppleClang) and Windows (MSVC), plus a coverage job using gcovr. Runs on every
  branch.
- **Header self-containment check** (`unit_tests/headers/`, enabled with
  `-DOPENTISSUE_ENABLE_HEADER_TESTS=ON`). Compiles every header on its own so that a header
  relying on some other header having been included first fails immediately rather than
  silently. `known_not_self_contained.txt` records the remaining exceptions and is a
  ratchet: entries may be removed, never added.
- **`OPENTISSUE_RANDOM_SEED`**, which pins the seed of the library's random generator.
  `ot_add_test()` sets it, making the tests that check numerical results over random inputs
  reproducible instead of sampling a different part of the error distribution on every run.
- **`OpenTissue::graphics`** is now an exported, installable target. Previously the graphics
  library could only be used from within this build tree.
- **`demos/opengl/gui_template`**, the minimal demo, ported to `GlfwApplication`. It is the
  worked template for restoring the other 28 demos that commit `dc6f0aa` removed; those
  sources remain in git history at `dc6f0aa^:demos/opengl/glut/`.
- **`grid_metaimage_write.h`**, which writes any grid as a MetaImage (`.mhd` plus `.raw`)
  for ParaView and other VTK-based tools. The header records the grid's origin and voxel
  spacing, so the volume loads at the position and scale the simulation used. Verified by
  reading the output back with VTK's own `vtkMetaImageReader`, the reader ParaView uses.
- `INSTALL.md`, `documentation/paraview.md` and this changelog.

### Changed

- **Dependency handling moved from Conan to plain CMake.** The Conan packages for TinyXML,
  Qhull, TetGen and Triangle were pinned to a private remote whose URL and credentials only
  ever existed as CI secrets, so no one outside the original organisation could resolve
  them. Every third-party library is now optional and a missing one no longer fails the
  configure step; `cmake/OpenTissueDependencies.cmake` reports what it found.
- **The project builds as C++17** (`CMAKE_CXX_STANDARD 17`, extensions off). The headers
  themselves remain standard-agnostic and are verified to compile at C++11, 14, 17 and 20,
  so `OpenTissue::headers` does not impose a standard on consumers.
- **TetGen and Triangle are off by default** and never downloaded. TetGen 1.5+ is AGPL-3.0
  and Triangle is licensed for non-commercial use only, neither of which sits with
  OpenTissue's zlib licence and its "free for commercial use" claim. Enabling them is now a
  deliberate choice. Note the TetGen *file format* readers parse plain text and need no
  library.
- **TinyXML is fetched and built from a pinned tarball** when it is not already installed,
  since it is dead upstream and rarely packaged.
- **GLUT is no longer pulled in by `graphics/core/gl/gl.h`.** Only the two text helpers need
  it, and they include `gl_glut.h` themselves.
- The PNG test fixture is committed to the repository. It was previously downloaded from
  Wikimedia at configure time, which made the build depend on a third-party site; that URL
  now returns HTTP 400.
- `cmake_minimum_required` raised to 3.25, and `CMP0167` set, since CMake 4 removed the
  bundled `FindBoost` module.
- **The programming guides now reference files that exist.** They had accumulated 120 dead
  source references, none of them from this work: the library had been reorganised from a
  flat layout into `core/`, `collision/` and `dynamics/`, and separately every header gained
  a `namespace_` prefix, and the guides followed neither change. 109 references were
  repointed at the file that actually holds the material. The 11 that remain are in two
  guides now marked as historical.

### Fixed

- **Roughly 200 headers were not self-contained.** They relied on older standard libraries
  transitively including `<iostream>`, `<vector>`, `<cassert>`, `<cmath>` and others. 892 of
  898 headers now compile standalone, up from 659, verified against both libc++ and
  libstdc++.
- **`image_read.h` overran its buffer on RGB images.** The channel count was read before the
  libpng transformations were configured, so an RGB image allocated three channels while
  libpng wrote four. Found by AddressSanitizer.
- **`math_random.h` violated the one-definition rule.** Whether the Boost or the `std::rand`
  implementation of `Random` was compiled depended on whether an earlier include happened to
  have pulled in Boost, so different translation units saw different definitions of the same
  class.
- **`is_finite` expanded to the legacy BSD `finite()`**, removed from POSIX.1-2008 and no
  longer declared by current C libraries, so every caller failed to compile.
- **`big_gmres.h` asserted on an absolute tolerance** that a correct Givens rotation cannot
  meet once the matrix is not of order one. `unit_gmres` had been failing outright.
- **The demos never started.** `glfw_window.cpp` requested `GLFW_OPENGL_FORWARD_COMPAT`
  without a context version, which GLFW rejects; the hint is wrong for this library in any
  case, since OpenTissue draws with the fixed-function pipeline that a forward-compatible
  context removes.
- Numerous smaller defects: a missing comma in a parameter list, a function that had lost
  its `template<>` declaration, `std::max`/`std::min` called with mismatched types,
  references to undeclared variables, a `ral_type` typo, two include cycles, and calls
  passing two template arguments to a single-parameter template.

### Removed

- **NVIDIA Cg and everything depending on it** — discontinued in 2012 with no build for
  current platforms. This takes GPU linear-blend skinning, GPU spherical-blend skinning, GPU
  tetrahedral scan conversion and a texture debug helper with it. The GPU skinning types
  were already commented out of the public API.
- **The ATLAS/LAPACK path**, which was guarded by a `USE_ATLAS` macro that no build ever
  defined and which depended on `boost/numeric/bindings`, never part of Boost and long
  unmaintained.
- **The GLUT application backend**, superseded by GLFW and referenced by nothing. GLUT
  itself is still required by the GL text helpers.
- `boost::shared_ptr`, `weak_ptr` and the pointer casts, replaced by their `std`
  equivalents; `boost/bind` and `boost/lambda`, replaced by lambdas and plain loops. uBLAS,
  `numeric_cast`, `multi_array` and `indirect_iterator` are still used and have no standard
  equivalent.
- `azure-pipelines.yml`, which targeted retired runner images and the removed Conan flow.
- `documentation/using_cmake.md`, a guide to generating Visual Studio 2005 project files,
  superseded by `INSTALL.md` and linked from nowhere.
- `documentation/volviz.md` and `documentation/using_demo_framework.md` are kept but marked:
  the first describes modules that no longer exist, the second the GLUT application
  framework that GLFW replaced.

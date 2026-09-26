# Changelog

All notable changes to OpenTissue are recorded here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

Anything older than the entry below predates this file; see the git history.

## [Unreleased]

### Added

- `demos/console/paraview_cantilever`, a structural simulation rendered in ParaView: a beam of
  soft rubber clamped at one end and released under its own weight, simulated with OpenTissue's
  finite element solver and written as legacy VTK files carrying each point's displacement and
  each element's von Mises stress. The beam droops far past what small-deflection beam theory
  describes, so it checks itself with measures that hold at any deflection -- its centre line
  stretches by at most 0.39% while its tip swings 2.6 m down -- and ships a `render.py` for
  ParaView. The guide covers it, including the material model and how a real material would
  compare, and how the result depends on the time step: `fem::simulate` runs a fixed 20
  conjugate gradient iterations, which is too few once the step is too large.
- `demos/console/paraview_shallow_water`, a physical simulation rendered in ParaView: a drop
  falling into a pool with a hill on its bottom, moved by OpenTissue's shallow water solver.
  It writes the sea bed and 101 frames of the water surface, prints checks that the water is
  behaving like water -- its volume holds to within 0.2% and the waves travel at the speed
  shallow-water theory predicts -- and ships a `render.py` for ParaView. The ParaView guide
  covers it, including how to write a height field.
- `ShallowWaterEquations::getSeaHeight()` and `getSeaBottom()`, the counterparts of the
  existing setters. The solver had no way to read its state back except by drawing it with
  OpenGL, so its results could not be exported or examined.
- `demos/console/paraview_export`, a complete, runnable example of getting OpenTissue data
  into ParaView: a mesh, its signed distance field, and an animated series of that field.
  It ships with `render.py`, which renders the output with ParaView's `pvbatch`; both have
  been run against ParaView 5.11.
- `unit_convex_hull`, the first test to actually *run* `mesh::convex_hull`. The polymesh and
  trimesh tests that mention it are compile-only -- they take a function's address and never
  call it -- so nothing verified that OpenTissue's use of Qhull produced a correct hull.
  It checks that interior points are dropped, that every extreme point survives, and that
  the result is a closed surface by Euler's formula.
- `unit_mbd_math_policy_equivalence`, the first test to actually *run* a multibody
  simulator; the existing mbd tests only compile one. It steps the ball-joint scene from the
  original multibody demo on both math policies and compares the trajectories, which agree to
  round-off, and checks the mass and inverse mass matrices entry by entry against the masses
  and inertias put in.
- `demos/console/benchmark_swe` and `demos/console/benchmark_lu`, which compare OpenTissue's
  uBLAS-based solvers with Eigen on identical input. They were written to decide whether
  porting `core/math/big/` to Eigen was worthwhile, and showed it was not: Eigen's conjugate
  gradient is no faster on the shallow-water system (0.9x), where assembly rather than the
  solve dominates each step; and while Eigen's dense LU is 11-12x faster, its only caller in
  OpenTissue is the variational interpolator, which nothing uses. They are the only code in
  the tree that uses Eigen, which OpenTissue itself does not depend on, and are built only
  when it is found.

### Changed

- The Doxygen API documentation (`OPENTISSUE_ENABLE_DOCUMENTATION`, target `apidoc`) is
  configured properly. `README.md` is the main page, and the guides under `documentation/`,
  `INSTALL.md` and `CHANGELOG.md` are pages of the site, with the README's links resolving to
  them. Every header gets a file page, not just the 2 of 899 that had a `\file` comment.
  Formulas render with MathJax instead of needing LaTeX. Paths are shown relative to the
  checkout instead of the builder's absolute paths, and undocumented members no longer warn,
  which cut the warnings from about 6,300 to about 650. Most of those remaining are real
  comment markup errors. The docs are no longer rebuilt on every build (`ALL` is gone), and
  configure warns when the option is on but Doxygen is missing, instead of silently creating no
  `apidoc` target.
- The documentation comments in about 180 headers and guides are fixed, cutting the Doxygen
  warnings from about 650 to 71. None of the remaining 71 is a markup error: 66 flag functions
  that document some of their parameters but not all, and 5 note include graphs too large to
  draw. The fixes, most damaging first:
  - LaTeX-style quotes (two backticks to open, two apostrophes to close) in 72 headers.
    Doxygen reads the two backticks as the start of a code span, so everything after them,
    often the rest of the file, vanished from the documentation. They are now plain double quotes.
  - Formulas with mismatched or reversed markers (`]\f` for `\f]`, `\f}` for `\f]`), and
    formulas written as raw LaTeX with no markers at all. `\norm` and `\mat`, which LaTeX does
    not define, are supplied through `documentation/formula_macros.tex`.
  - About 130 `@param` names that matched no parameter: misspellings (`@parma`, `intertia`),
    parameters renamed since (`n` for `n_val`), copy-paste leftovers (`map` for a grid), and
    parameters that are unnamed because they are unused, whose entries are removed.
  - Smaller slips: `@return` on functions returning `void`, code examples not marked as code,
    BibTeX entries whose `@` Doxygen took for a command, and a link to a guide that no longer
    exists.

  One comment described a bug rather than hiding one; see `grid::poisson_solver()` under
  Fixed.
- The ParaView guide is split into pages: `documentation/paraview.md` is now the entry page
  -- installing ParaView, running it from the command line, building the examples and the GUI
  basics -- and links to `paraview_writing_data.md`, `paraview_scripting.md` and one page per
  example. It had grown to over 500 lines covering three demos and the export reference.
- `grid::metaimage_write()` no longer prints a line on every successful write. It is typically
  called once per frame of an animation, so the message only buried a program's own output;
  failures are still reported on `std::cerr`.
- **The multibody engine now reaches its linear algebra only through its math policy.**
  Twenty mbd headers bypassed it for uBLAS API -- `vector_type::size_type`, `.clear()`,
  `.empty()`, writing into a sparse matrix with `operator()`, and an unqualified `prod` that
  only resolved through argument-dependent lookup -- so a policy could not in practice use
  anything but uBLAS types. They now go through the policy, which none of this changes for
  the two uBLAS policies.
  This came out of an experiment with an Eigen-backed math policy, dropped before release:
  it was 37x faster than `default_ublas_math_policy` on a 16000-row contact problem, but
  `optimized_ublas_math_policy`, which never forms the system matrix, was 1.5x faster than
  Eigen.

### Fixed

- **`grid::poisson_solver()` solved the wrong equation on grids with equal spacing.** Its
  branch for dx = dy = dz divided the Gauss-Seidel update by 8, where the seven-point
  discretization requires 6, so it converged to the solution of a different problem, and a zero
  right-hand side drove the field to zero instead of leaving a constant alone.
  `grid::laplacian_blur()`, which calls it, therefore also darkened the images it blurred: a
  uniform image lost a quarter of its brightness on every iteration. The branch for unequal spacing was correct. New test:
  `unit_poisson_solver`.
- **`documentation/paraview.md` gave instructions that did not work.** Its script used a
  `MetaFileReader` that ParaView does not have (`OpenDataFile()` or `MetaFileSeriesReader`),
  its signed distance field example called `mesh2phi(mesh, phi, 64)` as though 64 were the
  resolution when that overload treats it only as a cap -- a plain box came out at 16^3 --
  and its animation section did not say that every frame must share one grid. ParaView takes
  a series' grid geometry from its first file, so frames with differing grids are drawn
  distorted. All three are corrected, and the guide's snippets have now been run.
- **The spatial hash could hang for large tables.** `math_prime_numbers.h` computed
  products of residues in `int`, which overflows once the modulus passes 46341, so its
  Miller-Rabin test called nearly every larger prime composite and `modular_exponentiation`
  returned wrong, even negative, results. `prime_search()`, which sizes the spatial hash
  table used by mbd's broad phase, SPH and the versatile model, then scanned towards
  `INT_MAX` for a prime it could not recognise: `prime_search(100000)` never returned, so a
  multibody scene of that many bodies hung while setting up collision detection. The
  arithmetic is now 64-bit. The same function was also missing a pair of braces, so it
  miscounted the bits it walked, and shifted a signed `int` into its sign bit. New test:
  `unit_prime_numbers`.
- **`utility::Identifier` read past the end of a string literal.** It built each object's
  name as `"ID" + m_index` -- pointer arithmetic, not concatenation -- so from the fourth
  object on it read beyond `"ID"`. Every multibody body is an `Identifier`, so any scene
  triggered it, and AddressSanitizer stopped on the first one. New test: `unit_identifier`.
- **The Qhull-dependent code was silently skipped on Windows.** OpenTissue called Qhull's
  original, non-reentrant library, which keeps its state in globals; upstream deprecated it
  in favour of the reentrant `libqhull_r` and vcpkg now builds only the latter, so
  `find_package` came up empty and four test directories -- `vclip`, `polymesh`, `trimesh`
  and `multibody` -- were dropped while the job still reported success. `utility_qhull.h`
  and its three callers now use the reentrant API, which every supported platform provides.
  Qhull's own CMake config package is preferred over the bundled find module, since it
  carries per-configuration library locations that matter for a Debug build on Windows.
- **MSVC could not compile a translation unit that composes a multibody simulator**:
  `error C1128: number of sections exceeded object file format limit`. OpenTissue's template
  instantiations each need their own COMDAT section, and an assembled simulator goes past
  the 65,279 the default object format allows. `OpenTissue::headers` now carries `/bigobj`
  for MSVC. This had never been seen because the affected tests were the ones Qhull's absence
  was skipping on Windows.
- `t4mesh_delaunay_tetrahedralization.h` passed a null `FILE*` to `fprintf` when Qhull
  reported unfreed memory. That function sets `errfile` to 0, so the diagnostic path was
  undefined behaviour.
- `FindQhull.cmake` set `IMPORTED_LOCATION_RELEASE` from the *debug* library when both were
  present, so a release build could link the debug one.
- `unit_timer` asserted that a two-second `sleep()` returned in under 2.1 seconds. `sleep()`
  guarantees only that it will not return *early*, so that bound measured how busy the host
  was rather than anything about `Timer`; it failed on a CI runner that took 2.11 s. The
  upper bound is now 3.0 s, which still catches a stopped clock, a wrong unit or an
  uninitialised start.

## [1.0.0] - 2026-09-22

A modernisation pass. The project had not built out of the box for some years: its
dependencies came from a private package remote that no longer existed, and roughly a
quarter of its headers no longer compiled against current standard libraries.

The version moves from `0.994` to `1.0.0`. The changes below are breaking — Conan is gone,
the project builds as C++17, and Cg along with everything depending on it has been removed —
and the package config declares `SameMajorVersion` compatibility, so a consumer asking for
`find_package(OpenTissue 0.994)` will now correctly be told this is not that library.

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

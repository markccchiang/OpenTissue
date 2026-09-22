# Installing OpenTissue

OpenTissue is a header-only C++ library. There is nothing to build in order to *use* it —
you only need the headers on your include path and Boost available. A build is needed for
the unit tests, the demos, and the one compiled component (`OpenTissueGraphics`).

For how to consume OpenTissue from your own CMake project once it is installed, see
[Using OpenTissue in your Application](documentation/using_opentissue.md).

## Requirements

| | |
| --- | --- |
| Compiler | Anything supporting C++17: GCC, Clang/AppleClang, or MSVC 19+ |
| CMake | 3.25 or newer |
| Boost | 1.39 or newer |

Boost is the only hard requirement. OpenTissue itself builds as C++17, but the headers are
verified to compile at C++11, 14, 17 and 20 and do not impose a standard on your project.

## Dependencies

Everything except Boost is optional. A missing optional library never fails the configure
step; the headers and tests that need it are skipped, and a summary is printed at the end of
configure telling you what was found.

| Library | Needed for | Default |
| --- | --- | --- |
| Boost | most of OpenTissue; the tests also need `Boost::unit_test_framework` | **required** |
| TinyXML | the XML readers and writers under `*/io/` | ON, downloaded if absent |
| Qhull | `utility_qhull.h` — convex hulls and Delaunay tetrahedralisation | ON |
| libpng | `gpu/image/io/image_{read,write}.h` | ON |
| Eigen | `dynamics/mbd/math/mbd_eigen3_math_policy.h` only | ON |
| OpenGL, GLEW, GLFW, GLUT | `OpenTissueGraphics` and the demos | ON if found |
| TetGen | `t4mesh_tetgen_mesh_lofter.h` only | **OFF** |
| Triangle | `polymesh_compute_delaunay2D.h` only | **OFF** |

TinyXML is the only library OpenTissue will download for you. It is dead upstream and rarely
packaged, so it is fetched from a pinned tarball (checked against a SHA256) and built as part
of your build. Pass `-DOPENTISSUE_FETCH_DEPENDENCIES=OFF` to forbid that.

TetGen and Triangle are **off by default and never downloaded**: TetGen 1.5+ is AGPL-3.0 and
Triangle is licensed for non-commercial use only, neither of which is compatible with
OpenTissue's zlib licence. Turning them on is a deliberate choice you should make with their
terms in mind. Note that the TetGen *file format* readers parse plain text and do not need
the library.

### Installing the dependencies

These are the exact commands CI uses, so they are known to work.

**Debian / Ubuntu**

```sh
sudo apt-get install -y \
  libboost-dev libboost-test-dev \
  libqhull-dev libpng-dev libeigen3-dev \
  libglew-dev libglfw3-dev freeglut3-dev
```

**macOS (Homebrew)**

```sh
brew install boost qhull libpng glew glfw eigen
```

OpenGL and GLUT are system frameworks on macOS and need no installation.

**Windows (vcpkg)**

```sh
vcpkg install --triplet x64-windows \
  boost-algorithm boost-array boost-conversion boost-iterator \
  boost-lexical-cast boost-multi-array boost-numeric-conversion \
  boost-interval boost-optional boost-property-map boost-random \
  boost-test boost-type-traits boost-ublas boost-utility \
  qhull libpng glew glfw3 freeglut eigen3
```

`boost-test` alone is not enough; OpenTissue uses the other components listed above and
vcpkg ports them separately.

Eigen is optional and used by a single header: an alternative math policy for the multibody
engine (`mbd_eigen3_math_policy.h`). Note that Eigen requires C++14, so that one header raises
the standard the including translation unit needs. It is deliberately not linked into the
`OpenTissue::headers` target, so the rest of the library stays usable from C++11 -- link
`Eigen3::Eigen` yourself if you want the policy.

## Building

In-source builds are rejected, so always use a separate build directory.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```

On Windows, add the vcpkg toolchain file:

```sh
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=$VCPKG_INSTALLATION_ROOT/scripts/buildsystems/vcpkg.cmake
```

By default this builds almost nothing, because OpenTissue is header-only. Turn on what you
want:

| Option | Default | Effect |
| --- | --- | --- |
| `OPENTISSUE_ENABLE_UNIT_TESTS` | OFF | Build the test suite and register it with CTest |
| `OPENTISSUE_ENABLE_DEMOS` | OFF | Build the demo applications |
| `OPENTISSUE_ENABLE_HEADER_TESTS` | OFF | Check that every header compiles on its own |
| `OPENTISSUE_ENABLE_DOCUMENTATION` | OFF | Add the Doxygen `apidoc` target |
| `OPENTISSUE_ENABLE_CODE_COVERAGE` | OFF | Add `--coverage` (GCC/Clang; needs the unit tests on) |
| `OPENTISSUE_FETCH_DEPENDENCIES` | ON | Allow configure-time downloads |
| `OPENTISSUE_WITH_TINYXML` | ON | |
| `OPENTISSUE_WITH_QHULL` | ON | |
| `OPENTISSUE_WITH_EIGEN3` | ON | |
| `OPENTISSUE_WITH_TETGEN` | OFF | AGPL-3.0, see above |
| `OPENTISSUE_WITH_TRIANGLE` | OFF | non-commercial licence, see above |

## Running the tests

```sh
cmake -S . -B build -DOPENTISSUE_ENABLE_UNIT_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure -LE unreliable
```

Run the tests through `ctest` rather than invoking the binaries directly. `ctest` sets
`OPENTISSUE` and `OPENTISSUE_RANDOM_SEED` in the environment; without the latter the
library's random generator seeds itself from the clock, and the tests that check numerical
results over random inputs then fail intermittently.

`-LE unreliable` excludes two tests that do not reliably pass for any seed — `unit_kmeans`
and `unit_bfgs`, both of which depend on an iterative method converging from a random start.
Drop the flag to run them.

To check that every header compiles on its own:

```sh
cmake -S . -B build -DOPENTISSUE_ENABLE_UNIT_TESTS=ON -DOPENTISSUE_ENABLE_HEADER_TESTS=ON
cmake --build build --target opentissue_header_check -j
```

This adds roughly 900 translation units, which is why it is off by default.

## Installing

```sh
cmake --install build --prefix /where/you/want/it
```

This installs the headers, the CMake package files and, if it was built,
`libOpenTissueGraphics`.

## Troubleshooting

**"CMake generation for OpenTissue is not allowed within the source directory"** — use a
separate build directory, for example `cmake -S . -B build`.

**`graphics MISSING` in the dependency summary** — one of OpenGL, GLEW, GLFW or GLUT was not
found. The message names which. `OpenTissueGraphics` and the demos are skipped; the rest of
the library is unaffected.

**Tests skipped with "needs QHULL" or similar** — the corresponding library was not found.
Install it and re-run CMake.

**A header will not compile on its own** — check
`unit_tests/headers/known_not_self_contained.txt`, which lists the known exceptions and why.
If yours is not there, it is a regression worth fixing rather than adding to that file.

**Intermittent test failures** — you are probably running a test binary directly rather than
through `ctest`. See "Running the tests" above.

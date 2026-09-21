# Using OpenTissue in your Application
OpenTissue is a header-only library consisting of a collection of many smaller sub-libraries. In your application, you might want to use one of these libraries or several in combination with each other. Very much in the same way as you would use libraries like STL or Boost in an application.

Many of these sub-libraries are self-contained, that is, they do not have explicit third-party dependencies. Thus, in order to use OpenTissue in your application, you need to tell your compiler the location of the OpenTissue include headers which is usually accomplished by passing the flag (Linux, macOS) ```-I <path-to-opentissue>/``` at the command line. On the other hand, if you are using any sub-libraries that have some dependency then your application will have the same dependency. For instance, a substantial subset of the libraries uses Boost, so in these cases, your application will depend on Boost as well.

## Minimum requirements
  1. A C++ compiler supporting C++17 (GCC, Clang/AppleClang, or MSVC 19+).
  2. CMake 3.25 or newer.
  3. Boost 1.39 or newer.

Boost is the only hard dependency. Everything else is optional.

OpenTissue builds itself as C++17, but the headers do not require it: they compile cleanly at
C++11, 14, 17 and 20, and `OpenTissue::headers` does not force a standard on your project. If
your application is still on C++11 you can use OpenTissue as-is.

## Third-party dependencies

Most of OpenTissue is self-contained: of the ~900 headers, only a couple of dozen reach
out to a third-party library. Each of those libraries is optional, and a missing one never
breaks the configure step -- it just means the headers that use it are unavailable.

| Library  | Needed by | Default | Notes |
| -------- | --------- | ------- | ----- |
| Boost    | most of OpenTissue | **required** | Header-only except for the unit tests, which need `Boost::unit_test_framework`. |
| TinyXML  | the 14 XML readers/writers under `*/io/` | ON | Downloaded and built automatically if not installed, since it is dead upstream and rarely packaged. |
| Qhull    | `utility_qhull.h` (convex hull, Delaunay) | ON | Install it yourself: `brew install qhull`, `apt install libqhull-dev`, or `vcpkg install qhull`. |
| libpng   | `gpu/image/io/image_{read,write}.h` | ON | Found via the standard CMake `FindPNG` module. |
| TetGen   | `t4mesh_tetgen_mesh_lofter.h` only | **OFF** | AGPL-3.0. See the licensing note below. |
| Triangle | `polymesh_compute_delaunay2D.h` only | **OFF** | Non-commercial use only. See the licensing note below. |

The demos additionally need OpenGL, GLEW and GLFW.

Note that the TetGen *file format* readers (`t4mesh_tetgen_read.h` and friends) parse plain
text and do **not** require the TetGen library.

### A note on licensing

OpenTissue is zlib-licensed and free for commercial use. Two of the optional dependencies
are not compatible with that, so they are disabled by default and are never downloaded for
you. Turning them on is an explicit, informed choice:

  * **Triangle** (J.R. Shewchuk) is not open source. It is free for non-commercial use only
    and may not be sold for profit.
  * **TetGen** 1.5 and later is AGPL-3.0, which is strongly copyleft.

Enable them with `-DOPENTISSUE_WITH_TETGEN=ON` / `-DOPENTISSUE_WITH_TRIANGLE=ON` once you
have the libraries installed and have satisfied yourself that their terms suit your project.

## Workflow

OpenTissue uses CMake to generate build systems for Linux, macOS, and Windows. Being a
header-only library, the generated build system does not build anything unless you turn on
the demos or the unit tests. Instead it creates the target ```OpenTissue::headers``` for your
applications to link.

* Download OpenTissue:

      git clone https://github.com/erleben/OpenTissue.git

* Configure (in-source builds are rejected, so always use a separate build directory):

      cmake -S OpenTissue -B build

* Optionally build the tests and demos:

      cmake -S OpenTissue -B build -DOPENTISSUE_ENABLE_UNIT_TESTS=ON
      cmake --build build -j
      ctest --test-dir build

The configure step prints a summary of which optional dependencies it found, so you can see
at a glance what is and is not available in your build.

### Relevant options

| Option | Default | Effect |
| ------ | ------- | ------ |
| `OPENTISSUE_ENABLE_UNIT_TESTS` | OFF | Build the Boost.Test suite and register it with CTest. |
| `OPENTISSUE_ENABLE_DEMOS` | OFF | Build the demo applications. |
| `OPENTISSUE_ENABLE_DOCUMENTATION` | OFF | Add the Doxygen `apidoc` target. |
| `OPENTISSUE_FETCH_DEPENDENCIES` | ON | Allow downloading missing permissively-licensed dependencies (currently only TinyXML). Set to OFF for fully offline builds. |
| `OPENTISSUE_WITH_TINYXML` / `_QHULL` / `_TETGEN` / `_TRIANGLE` | ON / ON / OFF / OFF | Enable the headers gated on each library. |

## Using OpenTissue as Third-Party Software

After running CMake on the build directory a new configuration, target, and version files will be created:

      build/OpenTissueConfig.cmake
      build/OpenTissueTargets.cmake
      build/OpenTissueVersion.cmake

These files define OpenTissue's most important variables, settings and exported targets.

The following lines demonstrate how an application can link and use OpenTissue as an external library.

<pre>

MyProject
|
+-- CMakeLists.txt
|
+-- src
    |
    +-- main.cpp
</pre>

CMakeLists.txt:

<pre>
project(MyProject)

set(OpenTissue_ROOT "path-to-opentissue-build-dir")
find_package(OpenTissue)

add_executable(MyProject src/main.cpp)
target_link_library(MyProject
  PRIVATE
    OpenTissue::headers
)
</pre>

Notice that we set the ```OpenTissue_ROOT``` variable to the build directory containing ```OpenTissueConfig.cmake```. This will instruct ```find_package()``` to look there in order to search for OpenTissue. Once it finds ```OpenTissueConfig.cmake``` it will load it along with ```OpenTissueTargets.cmake``` and ```OpenTissueVersion.cmake```. This will automatically bring the ```OpenTissue::<target>``` targets which you can then use in your project to resolve OpenTissue dependencies in your application.

The special targets ```OpenTissue::<target>``` are ```interface external targets``` and they contain references to the OpenTissue headers, flags and possibly other flags that may be needed by your application. 

<table border="1">
<tr>
<td>Name</td><td>Usage</td>
</tr>
<tr>
<td>
OpenTissue::headers
</td>
<td>
External interface target. Contains references to OpenTissue header directory path, compiler flags and other definitions.
</td>
</tr>
</table>

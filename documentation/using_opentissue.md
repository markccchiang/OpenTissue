# Using OpenTissue in your Application
OpenTissue is a header-only library consisting of a collection of many smaller sub-libraries. In your application, you might want to use one of these libraries or several in combination with each other. Very much in the same way as you would use libraries like STL or Boost in an application.

Many of these sub-libraries are self-contained, that is, they do not have explicit third-party dependencies. Thus, in order to use OpenTissue in your application, you need to tell your compiler the location of the OpenTissue include headers which is usually accomplished by passing the flag (Linux, macOS) ```-I <path-to-opentissue>/``` at the command line. On the other hand, if you are using any sub-libraries that have some dependency then your application will have the same dependency. For instance, a substantial subset of the libraries uses Boost, so in these cases, your application will depend on Boost as well.

## Before you start

See [INSTALL.md](../INSTALL.md) for requirements, dependencies and how to build and install
OpenTissue. This page picks up from there and covers using it from your own project.

In short: OpenTissue needs a C++17 compiler, CMake 3.25+ and Boost. Everything else is
optional. The headers themselves are standard-agnostic and compile at C++11, 14, 17 and 20,
and `OpenTissue::headers` does not force a standard on your project, so a C++11 application
can use the library as-is.

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

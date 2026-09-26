[![CI](https://github.com/markccchiang/OpenTissue/actions/workflows/ci.yml/badge.svg)](https://github.com/markccchiang/OpenTissue/actions/workflows/ci.yml)
[![License](https://img.shields.io/badge/License-Zlib-blue.svg)](https://opensource.org/license/zlib)

# OpenTissue
OpenTissue is a collection of generic algorithms and data structures for rapid development of interactive modeling and simulation.

Free for commercial use, open source under the [Zlib license](https://opensource.org/license/zlib).

## Getting Started/Installing
  * [Installing OpenTissue](INSTALL.md) — requirements, dependencies, building and testing
  * [Using OpenTissue in your Application](documentation/using_opentissue.md)
  * [Changelog](CHANGELOG.md)

## For Developers
  * [Code Standards](documentation/code_standards.md)
  * [Good Practice for Development](documentation/good_practice.md)
  * [Design Patterns in OpenTissue](documentation/design_patterns.md)
  * [Unit Testing Guide](documentation/unit_testing.md)
  * [The Code Review Process](documentation/code_review.md)

## Read More
  * Have a look at our [OpenTissue Gallery](https://www.youtube.com/playlist?list=PLNtAp--NfuirWaf0HhB9wUeromoXWvJlb)
  * The API documentation, generated with Doxygen (see "Building the API documentation" in [INSTALL.md](INSTALL.md))
  * Demo applications (work as small tutorials/hands on examples)
  * The book: [Physics-Based Animation](https://iphys.wordpress.com/2020/01/12/free-textbook-physics-based-animation/)

## Programming Guides
Core- Atomic building blocks that are commonly used throughout all OpenTissue components, including data structures and algorithms.
  * [BIG Matrix-Vector Library (large scale)](documentation/big.md)
  * [The Mesh Programming Guide](documentation/mesh.md)
  * [The Tetrahedra Mesh Programming Guide](documentation/t4mesh.md)

Collision - Methods for detecting collisions between motion-independent objects and creating contact information.
  * [Optimal Spatial Hashing](documentation/hashing.md)
  * [Bounding Volume Hierarchy Data Structure](documentation/bvh.md)
  * [Signed Distance Field Collision Library](documentation/sdf.md)

Dynamics - Collection of methods for mathematically modeled simulations and physics-based animations, including rigid, soft, and fluid body dynamics.
  * [Multibody Body Dynamics](documentation/retro.md)
  * [Damped Wave Equations](documentation/dwe.md)

Kinematics - Methods for kinematic animations, including inverse kinematics, skinning, and key-framed character animation.
  * [Character Animation](documentation/character.md)

GPU - Collection of general purpose algorithms and methods performed on the GPU (GPGPU).
  * [The Image and Texture Programming Guide](documentation/texture.md)
  * [Visualising OpenTissue Data in ParaView](documentation/paraview.md)
  * [Volume Visualization](documentation/volviz.md) — *historical; the modules it describes were removed*

Utility - Large collection of miscellaneous utilities, mostly for OT-based applications.
  * [Using the Demo Application Framework](documentation/using_demo_framework.md) — *out of date; GLUT was replaced by GLFW*
  * [The OpenGL Programming Guide](documentation/using_opengl.md)
  * [Utility Programming Guide](documentation/utility.md)

## Learn More
Here are a few suggestions:
  * Learn more about generic programming from [C++ Templates: The Complete Guide](https://www.tmplbook.com/), by David Vandevoorde, Nicolai M. Josuttis and Douglas Gregor.
  * Learn about the C++ programming language from [The C++ Programming Language](https://www.stroustrup.com/4th.html), by Bjarne Stroustrup.
  * Learn more about solving problems with C++ from [Accelerated C++](https://www.informit.com/store/accelerated-c-plus-plus-practical-programming-by-example-9780201703535), by Andrew Koenig and Barbara E. Moo.

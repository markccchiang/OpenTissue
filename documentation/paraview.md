# Visualising OpenTissue Data in ParaView

OpenTissue's own OpenGL drawing is, by its authors' description, a set of brute-force debug
utilities. It is good for seeing whether a simulation is doing something sane while you
develop it. It is not good for producing a figure, inspecting a volume interactively, or
looking at a field in any detail.

[ParaView](https://www.paraview.org/) covers that second job. It is free and open source
(BSD), runs on Linux, macOS and Windows, and is built on VTK, so it understands volumetric
data properly: isosurfaces, volume rendering, slicing, thresholding, and scripted batch
rendering.

This guide covers the grid path, which is the one OpenTissue supports directly.

## Installing ParaView

Download a binary from [paraview.org/download](https://www.paraview.org/download/), or:

```sh
brew install --cask paraview     # macOS
sudo apt-get install paraview    # Debian/Ubuntu
```

## Trying it: the example program (on Mac)

`demos/console/paraview_export` is a complete example of everything in this guide. It builds
a box mesh, converts it to a signed distance field, and writes into the current directory:

- `box.obj` — the mesh
- `box_phi.mhd` and `box_phi.raw` — its signed distance field
- `spin_0000.mhd` … `spin_0009.mhd` (each with its `.raw`) — the field of the box turning, as
  an animation

Next to it is `render.py`, a ParaView script that renders those files to `box_phi.png`,
`spin_first.png` and `spin_last.png` without opening a window.

Run these commands from the root of the OpenTissue checkout. There are two ways to build it.

**Through CMake**, as part of the demos:

```sh
cmake -S . -B build-demos -DCMAKE_BUILD_TYPE=Release -DOPENTISSUE_ENABLE_DEMOS=ON
cmake --build build-demos --target paraview_export

cd build-demos/demos/console/paraview_export
./paraview_export
pvbatch render.py
```

The build copies `render.py` next to the program, so both commands run from the same
directory. `OPENTISSUE_ENABLE_DEMOS` requires the OpenGL, GLEW, GLFW and GLUT development
packages (see `INSTALL.md`), even though this demo does not draw anything itself.

**By compiling the one file directly**, which needs none of the graphics packages:

```sh
c++ -std=c++17 -O2 -I. -Ibuild -I/opt/homebrew/include \
    demos/console/paraview_export/src/paraview_export.cpp -o paraview_export

mkdir -p out && cd out
../paraview_export
pvbatch ../demos/console/paraview_export/render.py
```

- `-I.` finds the OpenTissue headers. Nothing needs linking: the library is header-only.
- `-Ibuild` finds `OpenTissue/configuration.h`, which CMake generates. It has to be a build
  directory that has been configured at least once (`cmake -S . -B build`).
- `-I/opt/homebrew/include` finds Boost when it was installed with Homebrew. Drop it where
  Boost is in a standard location, as it is on Debian and Ubuntu.
- The program writes into the current directory, hence `out/`.

`pvbatch` comes with ParaView, but a macOS install does not put it on your `PATH`. Either
call it by its full path, `/Applications/ParaView-<version>.app/Contents/bin/pvbatch`, or add
that directory to `PATH`:

```sh
export PATH="/Applications/ParaView-5.11.0.app/Contents/bin:$PATH"   # match your version
```

On Linux the `paraview` package installs `pvbatch` on the `PATH`; on Windows it is in the
`bin` folder of the ParaView installation.

### Exploring the output in the ParaView GUI

Start ParaView from the directory the program wrote to, so that **File → Open** starts there
-- `build-demos/demos/console/paraview_export` for the CMake build, `out` for the direct one:

```sh
cd build-demos/demos/console/paraview_export
/Applications/ParaView-5.11.0.app/Contents/MacOS/paraview
```

The GUI program is in `Contents/MacOS`, not in `Contents/bin` with `pvbatch`, so the `PATH`
change above does not reach it. Opening ParaView from the Dock and browsing to the directory
works just as well.

**1. The distance field and the mesh**

1. **File → Open**, choose `box_phi.mhd`, **OK**, then **Apply** in the Properties panel on
   the left. Only a bounding-box outline appears: that is the default for a volume.
2. With `box_phi.mhd` selected in the **Pipeline Browser** (top left), **Filters → Common →
   Contour**.
3. In Properties, set the value under **Isosurfaces** to **0**, then **Apply**. The box
   appears: this is the zero surface of the distance field.
4. **File → Open**, choose `box.obj`, **Apply**, and switch its representation in the
   toolbar drop-down from *Surface* to **Wireframe**. Its edges should sit exactly on the
   contour -- the check that the distance field is right.

**2. The values inside and outside**

1. Select `box_phi.mhd` in the Pipeline Browser again, **Filters → Common → Slice**,
   **Apply**.
2. In the toolbar's colouring drop-down, choose **MetaImage**. Negative values are inside the
   box, positive ones outside.
3. Drag the slice plane's arrow in the view to move the plane through the volume.

The eye icon beside each item in the Pipeline Browser hides or shows it.

**3. The animation**

1. **File → Open**. The ten numbered files show up as a single entry, `spin_..mhd`, with a
   small arrow beside it. Select that **group entry** -- not `spin_0000.mhd` inside it, which
   would load one frame only -- then **OK** and **Apply**.
2. **Filters → Common → Contour** at value **0** again, then **Apply**.
3. Press **▶** in the VCR controls on the toolbar at the top of the window: the box turns 9°
   per frame for ten frames. If the buttons are greyed out, only one time step was loaded --
   go back to step 1. If they are missing, **View → Toolbars → VCR Controls**.

Hide the objects from the first two parts with their eye icons so they do not overlap.

**Tips**

- Left-drag rotates the view, right-drag or the scroll wheel zooms, and middle-drag or
  Shift+drag pans. **Reset Camera** in the toolbar recentres everything.
- Nothing changes after editing a setting? Press **Apply** -- ParaView waits for it.
- **File → Save State** writes a `.pvsm` file; `paraview --state=yourfile.pvsm` brings the
  whole setup back, camera included.
- `paraview --script=render.py` builds the same scene as the script automatically and leaves
  it open to explore.

## Writing a grid

`grid_metaimage_write.h` writes any OpenTissue grid as a MetaImage, which ParaView reads
natively:

```cpp
#include <OpenTissue/core/containers/grid/grid.h>
#include <OpenTissue/core/containers/grid/io/grid_metaimage_write.h>

OpenTissue::grid::metaimage_write("phi", phi);
```

That produces two files:

- `phi.mhd` — a short text header giving the dimensions, voxel spacing, origin and element
  type. This is the file you open in ParaView.
- `phi.raw` — the voxel data, a plain binary dump in the grid's own layout.

The `.mhd` suffix on the name is optional; `metaimage_write("phi.mhd", phi)` does the same
thing. The header records the grid's `min_coord()` as the origin and its `dx()`, `dy()`,
`dz()` as the spacing, so the volume appears in ParaView at the position and scale the
simulation used, not in arbitrary voxel units.

### A worked example: a signed distance field

Signed distance fields are the most common thing worth looking at:

```cpp
#include <OpenTissue/core/containers/mesh/mesh.h>
#include <OpenTissue/core/containers/grid/grid.h>
#include <OpenTissue/core/containers/grid/util/grid_mesh2phi.h>
#include <OpenTissue/core/containers/grid/io/grid_metaimage_write.h>

typedef OpenTissue::math::BasicMathTypes<double, size_t>  math_types;
typedef OpenTissue::grid::Grid<float, math_types>         grid_type;

grid_type phi;
OpenTissue::grid::mesh2phi(mesh, phi, 0.25, 64);   // 0.25 margin around the mesh, 64 voxels per axis
OpenTissue::grid::metaimage_write("phi", phi);
```

Mind which `mesh2phi` overload you call. The shorter one, `mesh2phi(mesh, phi, 64)`, treats
64 only as an *upper limit*: it derives the resolution from the mesh's smallest face and
rounds it to a power of two, which for a plain box gives a 16x16x16 grid -- far too coarse to
look at. The four-argument form above uses the resolution you give it.

Open `phi.mhd` in ParaView and apply a **Contour** filter at value 0. That is the surface the
distance field represents, and comparing it against the mesh you started from is a direct
check that the conversion did what you expected.

## Using it in ParaView

1. **File → Open**, choose `phi.mhd`, then press **Apply** in the Properties panel. Nothing
   appears until you press Apply; this catches everyone once.
2. Pick a representation from the toolbar:
   - **Outline** — just the bounding box, the default for volumes.
   - **Volume** — direct volume rendering. Adjust the transfer function to see inside.
   - **Slice** — a single plane you can drag through the data.
3. Or apply a filter (**Filters → Common**):
   - **Contour** — isosurfaces. For a signed distance field, value 0 is the surface.
   - **Clip** / **Slice** — cut the volume open.
   - **Threshold** — keep only voxels in a value range.

The colour legend button shows the scalar range, and **Rescale to Data Range** is usually the
first thing worth pressing.

## Animations

Write one file per frame with a zero-padded number, and **keep the grid the same in every
frame** -- same origin, same spacing, same number of voxels:

```cpp
grid_type field;
field.create(min_corner, max_corner, I, J, K);   // once, big enough for the whole run

for(size_t frame = 0u; frame < frames; ++frame)
{
  simulator.run(timestep);
  // ... fill `field` for this frame ...

  std::ostringstream name;
  name << "phi_" << std::setw(4) << std::setfill('0') << frame;
  OpenTissue::grid::metaimage_write(name.str(), field);
}
```

ParaView groups numbered files automatically: the **File → Open** dialog lists them as a
single entry, `phi_..mhd`. Open that group entry rather than one of the files inside it, and
the VCR controls in the toolbar then play through the frames. **File → Save Animation**
writes the frames out as images or a movie.

The fixed grid matters because ParaView reads the grid geometry of a series from its *first*
file only and applies it to every other frame. If the origin or spacing changes from frame to
frame, the later frames are drawn with the wrong ones: a rotating box comes out as a sheared
parallelogram, although each file is correct when opened on its own.

A simulation that works on a fixed grid satisfies this without trying. Watch out for anything
that sizes the grid to its contents -- `mesh2phi()` does, fitting a new grid around the mesh
each call. To animate the distance field of a moving mesh, create the grid once and refill it
each frame with the scan conversion `mesh2phi()` uses internally:

```cpp
field.clear();                                            // back to "unused"
OpenTissue::mesh::compute_angle_weighted_vertex_normals(mesh);
OpenTissue::t4_cpu_scan(mesh, band, field, OpenTissue::t4_cpu_signed());
```

where `band` is how far from the surface to compute distances; make it large enough to cover
the grid. `demos/console/paraview_export` does exactly this.

## Scripting

ParaView ships `pvpython`, so a figure can be reproduced without clicking:

```python
from paraview.simple import *

phi = OpenDataFile('phi.mhd')
contour = Contour(Input=phi, ContourBy=['POINTS', 'MetaImage'], Isosurfaces=[0.0])
Show(contour)
Render()
SaveScreenshot('phi.png', ImageResolution=[1600, 1200])
```

`pvbatch` does the same without opening a window, which is what you want on a cluster or in
CI.

`OpenDataFile()` picks the right reader from the file name, which is sturdier across ParaView
versions than naming one. To load numbered frames as a time series, pass the list of files:

```python
import glob
series = MetaFileSeriesReader(FileNames=sorted(glob.glob('phi_*.mhd')))
scene = GetAnimationScene()
scene.UpdateAnimationUsingDataTimeSteps()
for t in series.TimestepValues:
    scene.AnimationTime = t
    Render()
```

`demos/console/paraview_export/render.py` is a complete script along these lines, run
against that demo's output.

## What this does not cover

- **One scalar field per file.** The writer takes a single grid. Several fields means several
  files, loaded separately and combined in ParaView with **Append Attributes**.
- **No vector fields.** OpenTissue stores vector-valued data as separate scalar grids; you
  would write each component and recombine with the **Calculator** filter.
- **No time metadata.** Frames are ordered by file name, not by simulation time. If the real
  timestamps matter, write a `.pvd` index file listing each file with its `timestep`.
- **Surface meshes go elsewhere.** For polygonal meshes use `mesh_obj_write.h`; ParaView reads
  OBJ directly, and so does Blender and MeshLab.

## Related exporters

| Header | Format | Use |
| --- | --- | --- |
| `grid_metaimage_write.h` | MetaImage `.mhd` + `.raw` | ParaView and other VTK tools; compact |
| `grid_matlab_write.h` | MATLAB `.m` | Small grids, numeric inspection; also runs in Octave |
| `grid_raw_write.h` | headerless binary | Data only, no dimensions; needs a header written by hand |
| `mesh_obj_write.h` | Wavefront OBJ | Surface meshes, for ParaView, Blender, MeshLab |
| `mesh_vrml_write.h` | VRML | Surface meshes, older tools |

`grid_metaimage_write.h` is essentially `grid_raw_write.h` plus the header that makes the data
self-describing, so prefer it unless something downstream specifically wants a bare dump.

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
OpenTissue::grid::mesh2phi(mesh, phi, 64);   // 64 is the resolution
OpenTissue::grid::metaimage_write("phi", phi);
```

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

Write one file per frame with a zero-padded number:

```cpp
for(size_t frame = 0u; frame < frames; ++frame)
{
  simulator.run(timestep);

  std::ostringstream name;
  name << "phi_" << std::setw(4) << std::setfill('0') << frame;
  OpenTissue::grid::metaimage_write(name.str(), phi);
}
```

ParaView groups numbered files automatically: opening `phi_0000.mhd` offers `phi_..mhd` as a
group, and the VCR controls in the toolbar then play through them. **File → Save Animation**
writes the frames out as images or a movie.

## Scripting

ParaView ships `pvpython`, so a figure can be reproduced without clicking:

```python
from paraview.simple import *

phi = MetaFileReader(FileName='phi.mhd')
contour = Contour(Input=phi, ContourBy=['POINTS', 'MetaImage'], Isosurfaces=[0.0])
Show(contour)
Render()
SaveScreenshot('phi.png', ImageResolution=[1600, 1200])
```

`pvbatch` does the same without opening a window, which is what you want on a cluster or in
CI.

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

# Writing Data for ParaView

How to get OpenTissue's data into files ParaView can open. The
[examples](paraview.md) put each of these into practice.

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

## Writing a height field

The water surface is a *height field*, one height for each (x, y) point, but an OpenTissue
grid is three-dimensional. [Waves in a pool](paraview_example_shallow_water.md) stores each height field in a grid only two layers
deep, one below every height and one above, holding *z minus the height*:

```cpp
field.create(vector3_type(0, 0, z_low), vector3_type(x_max, y_max, z_high), I, J, 2);
field(i, j, 0) = z_low  - height(i, j);
field(i, j, 1) = z_high - height(i, j);
```

That value is zero exactly at the surface and linear in z, so ParaView's **Contour** at
value 0 recovers the surface exactly -- the same idea as a signed distance field, and the
same step in ParaView. It also keeps the files small: two layers of 80 × 80.

The solver's heights are read with `getSeaHeight(i, j)` and `getSeaBottom(i, j)`.

## Writing a mesh with values on it

MetaImage holds grids, not meshes, so [the cantilever example](paraview_example_cantilever.md) writes **legacy VTK** files itself: a short
text format that ParaView reads natively and groups into a time series when the files are
numbered. An unstructured grid of tetrahedra with point and cell data is a few dozen lines to
write -- see `write_vtk()` in the demo:

```
# vtk DataFile Version 3.0
OpenTissue FEM cantilever
ASCII
DATASET UNSTRUCTURED_GRID
POINTS 625 double
...
CELLS 1920 9600          <- per tetrahedron: 4 and its four node indices
CELL_TYPES 1920          <- 10 for each, VTK's code for a tetrahedron
POINT_DATA 625
VECTORS displacement double
CELL_DATA 1920
SCALARS von_mises double 1
LOOKUP_TABLE default
```

The stress is computed the way the solver sees the material: the solver is *corotational*,
so it removes each element's rotation before measuring strain, and a beam swinging rigidly is
not counted as strained. `von_mises()` in the demo does the same.

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
the grid. `demos/console/paraview_export` does exactly this; see [its page](paraview_example_distance_field.md).

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
| `write_vtk()` in `paraview_cantilever` | legacy VTK `.vtk` | Tetrahedral meshes with point and cell values; demo code, not a library header |
| `mesh_vrml_write.h` | VRML | Surface meshes, older tools |

`grid_metaimage_write.h` is essentially `grid_raw_write.h` plus the header that makes the data
self-describing, so prefer it unless something downstream specifically wants a bare dump.

---

Part of [Visualising OpenTissue Data in ParaView](paraview.md).

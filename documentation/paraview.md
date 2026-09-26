# Visualising OpenTissue Data in ParaView

OpenTissue's own OpenGL drawing is, by its authors' description, a set of brute-force debug
utilities. It is good for seeing whether a simulation is doing something sane while you
develop it. It is not good for producing a figure, inspecting a volume interactively, or
looking at a field in any detail.

[ParaView](https://www.paraview.org/) covers that second job. It is free and open source
(BSD), runs on Linux, macOS and Windows, and is built on VTK, so it understands volumetric
data properly: isosurfaces, volume rendering, slicing, thresholding, and scripted batch
rendering.

These pages cover getting OpenTissue's results out of a simulation and into ParaView, with
three complete, runnable examples.

| Page | What it covers |
| --- | --- |
| [Writing data for ParaView](paraview_writing_data.md) | Grids, signed distance fields, height fields, meshes with values on them, animations, and what the exporters cannot do |
| [Scripting ParaView](paraview_scripting.md) | Rendering from a Python script, without opening a window |
| [Example: a signed distance field](paraview_example_distance_field.md) | A box and its distance field, rotating -- the export workflow end to end |
| [Example: waves in a pool](paraview_example_shallow_water.md) | A physical simulation: OpenTissue's shallow water solver |
| [Example: a cantilever beam](paraview_example_cantilever.md) | A structural simulation: OpenTissue's finite element solver, on a beam of soft rubber |

## Installing ParaView

Download a binary from [paraview.org/download](https://www.paraview.org/download/), or:

```sh
brew install --cask paraview     # macOS
sudo apt-get install paraview    # Debian/Ubuntu
```

## Running ParaView from the command line (on Mac)

`pvbatch` comes with ParaView, but a macOS install does not put it on your `PATH`. Either
call it by its full path, `/Applications/ParaView-<version>.app/Contents/bin/pvbatch`, or add
that directory to `PATH`:

```sh
export PATH="/Applications/ParaView-5.11.0.app/Contents/bin:$PATH"   # match your version
```

On Linux the `paraview` package installs `pvbatch` on the `PATH`; on Windows it is in the
`bin` folder of the ParaView installation.

The GUI program itself is elsewhere in the application bundle -- in `Contents/MacOS`, not in
`Contents/bin` with `pvbatch` -- so the `PATH` change above does not reach it. To start it from
a terminal, so that **File → Open** starts in the directory your program wrote to:

```sh
/Applications/ParaView-5.11.0.app/Contents/MacOS/paraview
```

Opening ParaView from the Dock and browsing to the directory works just as well.

## Building the examples

The examples are console demos under `demos/console/`, built with the rest of the demos. From
the root of the OpenTissue checkout:

```sh
cmake -S . -B build-demos -DCMAKE_BUILD_TYPE=Release -DOPENTISSUE_ENABLE_DEMOS=ON
cmake --build build-demos --target <demo name>
```

Each example's page gives the exact commands. Each demo writes its files into the current
directory, and the build copies its `render.py` next to it, so running the program and then
`pvbatch render.py` from its build directory needs no paths.

Run the configure line even if `build-demos` already exists. A build directory configured before
a demo was added has no rule for it, and `cmake --build` then stops with *No rule to make
target*; configuring again is safe and picks it up.

`OPENTISSUE_ENABLE_DEMOS` requires the OpenGL, GLEW, GLFW and GLUT development packages (see
`INSTALL.md`), even though these demos do not draw anything themselves.

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

### Tips

- Left-drag rotates the view, right-drag or the scroll wheel zooms, and middle-drag or
  Shift+drag pans. **Reset Camera** in the toolbar recentres everything.
- Nothing changes after editing a setting? Press **Apply** -- ParaView waits for it.
- **File → Save State** writes a `.pvsm` file; `paraview --state=yourfile.pvsm` brings the
  whole setup back, camera included.
- `paraview --script=render.py` builds the same scene as the script automatically and leaves
  it open to explore.

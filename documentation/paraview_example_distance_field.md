# Example: A Signed Distance Field (on Mac)

`demos/console/paraview_export` is a complete example of the export-and-view workflow. It builds
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

`pvbatch` is not on a Mac's `PATH` by default; see
[running ParaView from the command line](paraview.md#running-paraview-from-the-command-line-on-mac).

## Exploring the output in the ParaView GUI

Start ParaView from the directory the program wrote to, so that **File → Open** starts there
-- `build-demos/demos/console/paraview_export` for the CMake build, `out` for the direct one:

```sh
cd build-demos/demos/console/paraview_export
/Applications/ParaView-5.11.0.app/Contents/MacOS/paraview
```

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

For how the distance field and the animation are written, see
[writing a grid](paraview_writing_data.md#writing-a-grid) and
[animations](paraview_writing_data.md#animations).

---

Part of [Visualising OpenTissue Data in ParaView](paraview.md).

# Example: A Cantilever Beam

`demos/console/paraview_cantilever` simulates an elastic solid with OpenTissue's finite
element solver (`dynamics/fem/`). A beam of **soft rubber**, 4 m long and 0.5 m square and
made of 1920 tetrahedra, is clamped to a wall at one end and released under its own weight.
It droops a long way, swings, and settles hanging down, bent in a curve.

It writes one file per frame, `beam_0000.vtk` … `beam_0100.vtk`, 0.06 s apart, holding the
deformed mesh with each point's **displacement** from rest and each element's **von Mises
stress** -- the usual single measure of how hard the material is being worked.

```sh
cmake -S . -B build-demos -DCMAKE_BUILD_TYPE=Release -DOPENTISSUE_ENABLE_DEMOS=ON
cmake --build build-demos --target paraview_cantilever
cd build-demos/demos/console/paraview_cantilever
./paraview_cantilever
pvbatch render.py
```

`render.py` saves `beam_0000.png`, `beam_0010.png` … `beam_0100.png`: the beam coloured by
stress, darker where it is higher, against the outline of where it started. Nothing is
exaggerated; soft rubber really droops that far.

## The material

The material is defined by three numbers, set at the top of the demo and passed to
`fem::init()`:

| Property | Value | What it means |
| --- | --- | --- |
| Young's modulus | 5 × 10⁶ Pa (5 MPa) | Stiffness: how much stress it takes to stretch the material |
| Poisson's ratio | 0.49 | How much it narrows sideways when stretched. Rubber is nearly incompressible; 0.5 is the limit |
| Density | 1100 kg/m³ | The beam weighs 1100 kg |

The model also assumes:

- **Isotropic** -- equally stiff in every direction; no grain, as wood would have.
- **Homogeneous** -- every element gets the same values.
- **Linear elastic** -- stress proportional to strain (Hooke's law), and it springs fully
  back. Plasticity is switched off by passing an unreachable yield value (`1.0e30`) to
  `fem::init()`.
- **Large rotations handled** -- the solver is *corotational*: it removes each element's
  rotation before measuring strain, so bending a long way is not mistaken for stretching.
- **Damping** of 2 × mass, hard-coded in `fem_simulate.h`; the demo cannot change it. It is
  why the beam settles within a few seconds.

For a sense of scale, this is the same 4 m beam in other materials, as small-deflection beam
theory predicts it (tip sag δ = ρgAL⁴ / 8EI, and the period of its first mode of vibration):

| Material | Young's modulus (Pa) | Density (kg/m³) | Tip sag | Swing period |
| --- | --- | --- | --- | --- |
| **soft rubber (this demo)** | 5 × 10⁶ | 1100 | 3.3 m\* | 2.9 s |
| polyethylene (LDPE) | 2 × 10⁸ | 920 | 69 mm | 0.42 s |
| pine, along the grain | 1 × 10¹⁰ | 500 | 0.75 mm | 0.044 s |
| aluminium | 7 × 10¹⁰ | 2700 | 0.58 mm | 0.039 s |
| steel | 2 × 10¹¹ | 7850 | 0.59 mm | 0.039 s |

\* Far beyond what small-deflection theory can describe: it assumes the sag is a few percent
of the length, not 83% of it. The simulation shows what actually happens, below.

A steel beam this size would sag less than a millimetre and vibrate at about 25 Hz -- it would
render as a beam that does not move, which is why the demo uses rubber.

**Changing the material** means changing those three constants, and very likely the time
step with them. Stiffer materials vibrate much faster, and the solver runs a fixed 20
conjugate-gradient iterations per step (`fem_simulate.h`), which is not enough once the step
is too large for the material. With rubber at a Poisson's ratio of 0.49, a step of 0.005 s
already gives a wrong answer -- the first swing comes out 2.77 m deep at 1.75 s instead of
2.60 m at 1.36 s -- while 0.0025 s and 0.001 s agree. The demo uses 0.0025 s. For steel, which
vibrates about 75 times faster than this rubber, expect to need a far smaller step. Whatever
you change, check the result against a smaller step before trusting it.

## What it prints

```
small-deflection beam theory would predict a 3.31 m sag -- 83% of the length, far
beyond the few percent it is valid for. The checks below hold at any deflection.

  time   tip down   tip out   centre line   largest stress
  0.00    0.000 m    4.000 m    4.0000 m           0 Pa
  1.20    2.560 m    2.901 m    4.0154 m      946065 Pa
  ...
  6.00    2.157 m    3.262 m    4.0112 m      739606 Pa

deepest point 2.600 m below the clamp at 1.36 s; settled 2.157 m down and 3.262 m out,
3.911 m from the clamp in a straight line (the beam is 4.0 m long, and bent)
centre line stayed between 4.0000 m and 4.0156 m long: stretched by at most 0.39%
```

- **The beam droops past where theory stops.** Its first swing takes the tip 2.60 m below the
  clamp; it settles 2.16 m down and 3.26 m out, 3.91 m from the clamp in a straight line -- as
  it should be for a 4 m beam that has bent without stretching.
- **The centre line keeps its length**, stretching by at most 0.39%. Bending a long way
  should not stretch the middle of a beam; that it does not shows the large rotations are
  being handled rather than mistaken for strain. The small stretch there is is physical: the
  part hanging down is pulled along its length by its own weight, which for rubber this soft
  is a strain of a few tenths of a percent.
- **The stress is highest along the top and bottom surfaces next to the wall**, where bending
  is greatest, and falls to nothing towards the free end and along the middle of the beam.
  The checkerboard look comes from each tetrahedron carrying a single stress value.

Simple (linear) tetrahedra are too stiff in bending, an effect known as *locking*, and with a
Poisson's ratio close to 0.5 also in compression (*volumetric locking*). Here that is modest:
at 0.3 instead of 0.49 the tip settles 2.32 m down instead of 2.16 m. Both shrink as the mesh
is refined.

The files are legacy VTK, written by the demo itself; see
[writing a mesh with values on it](paraview_writing_data.md#writing-a-mesh-with-values-on-it)
for the format, and for how the stress is computed.

## In the ParaView GUI

1. **File → Open** and choose the group entry `beam_..vtk`, then **Apply**.
2. In the toolbar, colour by **von_mises** and set the representation to **Surface With
   Edges** to see the elements.
3. Press **▶** in the VCR controls to watch it swing and settle.
4. To see where the beam started, open `beam_0000.vtk` on its own (not the group) and show it
   as **Outline**. To exaggerate the motion instead, apply **Filters → Alphabetical → Warp By
   Vector** to the group, using `displacement` with a **Scale Factor** above 1 -- remembering
   that the file's coordinates are already the deformed ones, so a factor of 1 doubles the
   deflection.

---

Part of [Visualising OpenTissue Data in ParaView](paraview.md).

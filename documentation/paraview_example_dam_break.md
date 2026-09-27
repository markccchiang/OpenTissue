# Example: A Dam Break

`demos/console/paraview_dam_break` is a multi-particle simulation. A column of water 0.3 m
long and 0.44 m high stands at one end of a tank 1 m long, held by a dam, and at t = 0 the
dam is gone. The column collapses, a wave runs along the floor, climbs the far wall, falls
back, and the water sloshes to rest.

The water is 1,232 particles, moved by OpenTissue's smoothed particle hydrodynamics solver
(`dynamics/sph/`). SPH treats a fluid as particles, each carrying a small mass of it: the
density at a particle is a weighted sum over its neighbours within a *smoothing radius*,
the pressure follows from the density, and pressure differences and viscosity push the
particles around. There is no grid and no mesh, which is why SPH handles splashes and a
free surface that breaks up so naturally.

It writes, into the current directory:

- `tank.vtk` — the tank, as the twelve edges of a box, written once
- `water_0000.vtk` … `water_0150.vtk` — the particles, 151 frames 0.02 s apart, each with
  its velocity, speed, density and pressure

Build and run it [like the other examples](paraview.md#building-the-examples); `render.py`
here saves `water_0000.png`, `water_0010.png` … `water_0150.png`, six moments from the
release to rest:

```sh
cmake -S . -B build-demos -DCMAKE_BUILD_TYPE=Release -DOPENTISSUE_ENABLE_DEMOS=ON
cmake --build build-demos --target paraview_dam_break
cd build-demos/demos/console/paraview_dam_break
./paraview_dam_break
pvbatch render.py
```

The simulation takes a few seconds. While it runs, the program prints checks that the water
is behaving like water:

```
1232 particles, spacing 0.0272 m, smoothing radius 0.0457 m, time step 0.0025 s
  time   front   top     outside   density: mean   lowest   highest   mean speed
  0.00   0.285   0.421      0        889.6        619.3     961.2     0.000
  0.25   0.660   0.293      0        996.4        327.6    1193.3     1.023
  0.50   0.986   0.459      0        968.7        338.5    1308.6     0.993
  ...
  3.00   0.986   0.103      0       1014.0        848.7    1056.3     0.019
```

The numbers differ a little from run to run: the order in which the solver sums a particle's
neighbours depends on where the particles sit in memory, and in a splashing flow those
rounding differences grow. The checks hold on every run.

- **outside** counts particles that have left the tank. It must stay zero.
- **density** should stay close to that of water, 998 kg/m³, since water barely compresses.
  From the release on, the mean stays within 4% of it. It starts lower, at 890, because the
  column is small: about half its particles are on its surface, with fewer neighbours to sum
  over. The *lowest* values belong to spray and to such surface particles; the *highest* are
  momentary, where the water is squeezed against the floor and the far wall.
- **mean speed** falls towards zero as the sloshing dies out: the water comes to rest.
- **front** is where the wave front is along the floor; it reaches the far wall, at 0.986 m
  (the tank's 1 m less half a particle spacing), before 0.5 s. **top** is the highest
  particle, which climbs above 0.55 m as the wave runs up the far wall.

## What the program sets up, and why

Most of the program is assembling the solver, which is built from policies: the neighbour
search, the kernels, the pressure law, the forces and the integrator are all template
arguments of `sph::System`. A few choices matter for getting water that behaves:

- **Neighbour search.** The program defines `SPHSH` before including `sph.h`, which makes
  the solver find neighbours with a spatial hash grid. Without it, every particle is summed
  against every other one -- the same answer, about eight times more slowly here.
- **Kernel range check.** The kernels take a `CheckRange` flag. With it on, a kernel returns
  zero beyond the smoothing radius. That matters: beyond it, the density kernel
  (h² − r²)³ is *negative*, and summing it over far-away particles gives densities in the
  negative tens of billions.
- **Pressure law.** The solver's pressure is k (ρ − ρ₀), which turns negative wherever the
  density is below rest -- at the free surface and next to walls, where a particle has
  fewer neighbours. Negative pressure pulls particles together, and the water packs itself
  onto the floor in fewer, denser layers than its volume allows. The pressure law is a
  policy, so the program supplies its own, `ClampedPressure`, which never goes below zero.
- **Walls.** The solver keeps particle *centres* inside the tank, but a particle stands for
  water half a spacing around its centre. The box the solver sees is therefore half a
  spacing smaller on every side than the tank drawn in ParaView.
- **Stiffness and time step.** SPH water is slightly compressible, by how much set by its
  gas stiffness, and a stiffer fluid needs a shorter time step. OpenTissue's `Water`
  pairs a stiffness of 3 with a 0.01 s step, which lets the column squeeze to twice its
  density as it collapses. The program's `StiffWater` uses a stiffness of 40 with a 0.0025 s
  step, which keeps it within a few percent of the density of water, for four times the
  steps.
- **Starting lattice.** The particles start on a cubic lattice at water's rest spacing,
  ∛(particle mass / density) = 2.7 cm, so the column starts close to rest density rather
  than with a jolt.

One thing the program does not change: the material's viscosity is 3.5 Pa·s, about 3,500
times that of real water. Like most SPH, OpenTissue's relies on a high viscosity to stay
stable. It makes the water settle within a few seconds; real water in this tank would slosh
for much longer.

## In the ParaView GUI

1. **File → Open** `tank.vtk`, **Apply**.
2. **File → Open** and choose the group entry `water_..vtk`, **Apply**.
3. With the water selected, set the representation in the toolbar to **Point Gaussian**.
   In the Properties panel set **Gaussian Radius** to `0.0136` -- half the particle
   spacing -- and **Shader Preset** to *Sphere*. Each particle is now drawn as a small
   shaded sphere; at the default representation, *Points*, they are single pixels.
4. Colour by `speed`. The pictures `render.py` saves use the *Blues* map, inverted so that
   still water is near white and fast water dark blue, over a fixed range of 0 to 2.5 m/s
   so that frames can be compared.
5. Press **Play**. Colouring by `density` or `pressure` instead shows where the water is
   squeezed: along the floor under the collapsing column, and where the wave meets the far
   wall.

With so few particles, each one is visible. For a smoother look, ParaView's
**Filters → Alphabetical → SPH Volume Interpolator** can resample the particles onto a grid,
which can then be contoured into a surface, at the cost of a slower pipeline.

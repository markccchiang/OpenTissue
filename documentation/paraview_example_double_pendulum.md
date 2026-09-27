# Example: A Double Pendulum

`demos/console/paraview_double_pendulum` is a multibody simulation. Two rigid rods, each 1 m
long and 1 kg, are joined end to end by hinges, the upper one to a fixed pivot. Both start
horizontal and at rest, and OpenTissue's multibody engine (`dynamics/mbd/`) lets them fall
for ten seconds. The rods are rigid bodies, the hinges are joint constraints, and at every
step the engine's constraint solver works out the forces that keep the joints together.

A double pendulum is the classic example of a chaotic system: however small a difference in
how it starts, or in how it is computed, the difference grows exponentially until the motion
is completely different. So the program also integrates the pendulum's equations of motion
directly, far more accurately than a general-purpose engine can, and writes that *reference*
pendulum alongside the engine's. The pictures show both: the engine's in orange, the
reference as a faint grey ghost. For seven seconds the grey one hides behind the orange one;
then they part.

It writes, into the current directory:

- `pendulum_0000.vtk` … `pendulum_0250.vtk` — the engine's pendulum, 251 frames 0.04 s apart:
  the pivot, the elbow and the tip, joined by two lines, one per rod
- `reference_0000.vtk` … — the reference pendulum, the same way
- `trace_0000.vtk` … — the path of the engine pendulum's tip so far, with the time at each
  point

Build and run it [like the other examples](paraview.md#building-the-examples); `render.py`
here saves `pendulum_0000.png`, `pendulum_0025.png` … `pendulum_0250.png`, six moments from
the release to ten seconds:

```sh
cmake -S . -B build-demos -DCMAKE_BUILD_TYPE=Release -DOPENTISSUE_ENABLE_DEMOS=ON
cmake --build build-demos --target paraview_double_pendulum
cd build-demos/demos/console/paraview_double_pendulum
./paraview_double_pendulum
pvbatch render.py
```

It takes under a second. While it runs, the program prints the engine against the reference
(the two `SpatialHashing::init()` lines before it come from the engine's collision detection):

```
time step 0.0025 s; angles from straight down, in radians
  time   upper rod: engine  reference   lower rod: engine  reference   energy change   joint gap
   0.0         1.571      1.571              1.571      1.571         +0.000 J     6.1e-17 m
   0.5         0.445      0.448              1.035      1.042         -0.052 J     7.8e-05 m
   1.0        -1.212     -1.208             -0.815     -0.814         +0.061 J     2.3e-05 m
   ...
   6.5        -0.810     -0.810             -2.275     -2.278         -0.014 J     4.8e-05 m
   7.0         1.122      1.092             -1.811     -1.935         +0.032 J     3.3e-05 m
   7.5         1.151      1.345              0.439     -0.140         -0.068 J     1.2e-04 m
   ...
  10.0        -1.111     -0.744             -0.146     -1.779         +0.038 J     8.4e-05 m
```

- **The angles** agree to within 0.02 rad for six and a half seconds, through a dozen
  swings. After that they part, and by ten seconds they have nothing to do with each
  other. That is chaos, not a failure: the engine's small errors grow exponentially, as a
  double pendulum's do.
- **The energy change** should be zero, since nothing here loses energy. It stays within
  0.07 J -- a third of a percent of the 19.6 J the pendulum gains falling from horizontal to
  hanging straight down. The error comes from the constraint solver, which runs a fixed number
  of iterations, and from the correction that pulls drifting joints back together.
- **The joint gap** is how far apart the two halves of a hinge have drifted; under a third of
  a millimetre throughout.

The run is deterministic: the same numbers every time.

### Accuracy and the time step

The time step decides how long the engine keeps up with the reference, and how well it keeps
the energy. Measured with this program:

| Time step | Agrees within 0.1 rad until | Largest energy change |
| --- | --- | --- |
| 0.01 s | 5 s | 0.79 J |
| 0.005 s | 7 s | 0.23 J |
| 0.0025 s | 7 s | 0.07 J |
| 0.001 s | 8.5 s | 0.03 J |

The energy error falls steadily as the step shrinks, but the time the two agree grows only
slowly. That is what exponential growth of errors means: a smaller error only postpones the
moment it becomes large, by a little, so no finite accuracy predicts a double pendulum for
long. Raising the solver's iterations from 30 to 100 at a 0.0025 s step made little
difference (7.5 s instead of 7 s, the same energy error); the time step is what limits it.

## How the pendulum is built

The engine assembles a simulation from policies -- the math policy, the stepper and its
solver, the collision detection, the simulator -- as template arguments of `mbd::Types`, the
same pattern as the unit test `unit_mbd_math_policy_equivalence`. The scene itself is:

- **Bodies.** A fixed pivot and two rods. Each rod's body frame has the rod along x, so its
  orientation is a turn about z, and its mass properties are set directly: 1 kg, and the
  moment of inertia of a uniform rod, m L² / 12, about its centre.
- **Sockets and hinges.** A joint connects two *sockets*, each a coordinate frame fixed in a
  body. A hinge turns about the z axis of its sockets' frames. With the pendulum swinging in
  the x-y plane under gravity along -y, the hinges turn about world z and every socket frame
  is simply unrotated, placed at the end of a rod.
- **No collisions.** On a real double pendulum the arms swing in planes a few centimetres
  apart, so the lower one passes the pivot freely. The program does the same: the pivot's
  block sits behind the upper rod's plane and the lower rod in front of it, offsets along the
  hinges' axis that do not change the motion. The engine skips collisions between bodies that
  share a joint, so nothing ever collides. Without the offsets, the lower rod reaches the
  pivot's block after six or seven seconds, and the engine stops with an exception: this
  configuration binds no handler for a collision between two boxes.
- **Declaration order.** The configuration refers to the bodies and joints and tears them down
  in its destructor, so they are declared before it and outlive it.

The reference is the textbook result for two identical uniform rods, derived with Lagrangian
mechanics (see, for example, Wikipedia's "Double pendulum"), in the rods' angles and their
generalized momenta. It is integrated with fourth-order Runge-Kutta, a hundred steps to each
of the engine's.

## In the ParaView GUI

1. **File → Open** and choose the group entry `trace_..vtk`, **Apply**. Colour it by `time`.
2. **File → Open** the group `pendulum_..vtk`, **Apply**, then **Filters → Alphabetical →
   Tube**, set **Radius** to `0.025`, **Apply**. This makes the two lines into rods.
3. To mark the pivot, elbow and tip, select `pendulum_..vtk` again, make it visible, set its
   representation to **Point Gaussian**, **Gaussian Radius** `0.05` and **Shader Preset**
   *Sphere*.
4. **File → Open** the group `reference_..vtk`, **Apply**, add a **Tube** with a thinner
   radius, and give it a grey solid colour and an **Opacity** of about `0.35`.
5. Set the view to look along -z with y up (the **-Z** button in the camera toolbar), and
   press **Play**.

# Example: Waves in a Pool

`demos/console/paraview_shallow_water` runs a real simulation. A drop of water falls into a
pool 10 units across and 1 unit deep, with a hill on the bottom, and OpenTissue's shallow
water solver (`dynamics/swe/`) moves the water for six seconds. The rings spread out, bounce
off the walls, and slow down where they cross the shallow water over the hill: in shallow
water, waves travel at √(g · depth).

It writes, into the current directory:

- `sea_bed.mhd` / `.raw` — the bottom of the pool, written once since it does not move
- `water_0000.mhd` … `water_0100.mhd` — the water surface, 101 frames 0.06 s apart

Build and run it [like the other examples](paraview.md#building-the-examples); `render.py` here saves `water_0000.png`,
`water_0008.png` … `water_0100.png`, six moments from the drop to the end:

```sh
cmake -S . -B build-demos -DCMAKE_BUILD_TYPE=Release -DOPENTISSUE_ENABLE_DEMOS=ON
cmake --build build-demos --target paraview_shallow_water
cd build-demos/demos/console/paraview_shallow_water
./paraview_shallow_water
pvbatch render.py
```

While it runs, the program prints checks that the water is behaving like water:

```
  time   volume    change   highest   crest distance
  0.00   96.8899  +0.0000%   1.2500    0.000
  0.50   96.8816  -0.0085%   1.0365    1.875
  1.00   96.8612  -0.0296%   1.0442    3.500
  1.50   96.8337  -0.0580%   1.0366    4.875
  ...
```

- **volume** should stay constant, since no water enters or leaves the pool. It drifts by
  0.2% over the six seconds; that is the solver's numerical scheme, which is stable but not
  exactly conservative.
- **crest distance** is how far the ring's crest has travelled from the drop towards the far
  wall. Between 0.5 s and 1.5 s it covers 3.0 units a second, against √(9.81 × 1) = 3.13 for
  shallow water 1 unit deep. It reaches the wall 5 units away at about 1.6 s, reflects, and is
  back over the drop at 3 s.

The water surface and the sea bed are height fields, which need a trick to fit into
OpenTissue's three-dimensional grids; see
[writing a height field](paraview_writing_data.md#writing-a-height-field). The solver's
heights are read with `getSeaHeight(i, j)` and `getSeaBottom(i, j)`.

## In the ParaView GUI

1. **File → Open** `sea_bed.mhd`, **Apply**, then **Filters → Common → Contour** at value
   **0**, **Apply**. Set its colouring to *Solid Color*.
2. **File → Open** and choose the group entry `water_..mhd`, **Apply**, then **Contour** at
   **0** again.
3. With that contour selected, **Filters → Common → Calculator**, set **Result Array Name**
   to `elevation` and the expression to `coordsZ - 1`, **Apply**. This is the height above
   the water at rest; colour by it, with a diverging colour map centred on zero.
4. The waves are a few hundredths of a unit high, so they look flat at true scale. For each
   of the two surfaces, click the gear icon in Properties to show the advanced options, and
   under **Transforming** set **Scale** to `1 1 3`. The pictures `render.py` saves use the
   same exaggeration, and say so.
5. Press **▶** in the VCR controls. **File → Save Animation** writes the frames out as images
   or a movie.

---

Part of [Visualising OpenTissue Data in ParaView](paraview.md).

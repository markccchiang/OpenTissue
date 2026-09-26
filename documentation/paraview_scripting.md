# Scripting ParaView

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

Each example ships a complete script along these lines, `render.py`, run against that demo's
output: [the distance field](paraview_example_distance_field.md),
[waves in a pool](paraview_example_shallow_water.md) and
[the cantilever beam](paraview_example_cantilever.md). See
[running ParaView from the command line](paraview.md#running-paraview-from-the-command-line-on-mac)
for where `pvbatch` lives on a Mac.

---

Part of [Visualising OpenTissue Data in ParaView](paraview.md).

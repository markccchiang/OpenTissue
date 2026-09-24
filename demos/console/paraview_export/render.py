#
# Renders the output of paraview_export with ParaView, without opening a window:
#
#   pvbatch render.py
#
# Run it in the directory paraview_export wrote its files to. It saves:
#
#   box_phi.png      the zero isosurface of the distance field, the original mesh as a
#                    wireframe on top of it, and a slice coloured by distance
#   spin_first.png   the first and last frames of the animation, loaded as one time series
#   spin_last.png
#
from paraview.simple import *
import glob

# OpenDataFile() picks the MetaImage reader from the file name.
phi = OpenDataFile('box_phi.mhd')
phi.UpdatePipeline()
array = phi.PointData.keys()[0]                 # 'MetaImage'

view = GetActiveViewOrCreate('RenderView')
view.Background = [1, 1, 1]
try:
    view.UseColorPaletteForBackground = 0       # ParaView 5.10 and later
except AttributeError:
    pass

# The zero isosurface of a signed distance field is the surface it encodes.
surface = Contour(Input=phi, ContourBy=['POINTS', array], Isosurfaces=[0.0])
surface_display = Show(surface, view)
surface_display.Opacity = 0.6

# The mesh it was computed from, as a wireframe: the two should coincide.
mesh = OpenDataFile('box.obj')
mesh_display = Show(mesh, view)
mesh_display.SetRepresentationType('Wireframe')
mesh_display.AmbientColor = mesh_display.DiffuseColor = [0.1, 0.1, 0.1]
mesh_display.LineWidth = 3.0

# A slice through the middle, coloured by distance: blue inside, red outside.
cut = Slice(Input=phi)
cut.SliceType.Normal = [0, 0, 1]
cut.SliceType.Origin = [0, 0, 0]
cut_display = Show(cut, view)
ColorBy(cut_display, ('POINTS', array))
lut = GetColorTransferFunction(array)
lut.ApplyPreset('Cool to Warm', True)
low, high = phi.PointData[array].GetRange()
extent = max(abs(low), abs(high))
lut.RescaleTransferFunction(-extent, extent)     # keep zero in the middle of the colour map

view.CameraPosition = [2.6, -3.2, 2.4]
view.CameraFocalPoint = [0, 0, 0]
view.CameraViewUp = [0, 0, 1]
Render(view)
SaveScreenshot('box_phi.png', view, ImageResolution=[1400, 900])
print('saved box_phi.png')

# The animation. Pass the files as one list to read them as a time series. They share one
# grid, which matters: ParaView takes a series' grid geometry from its first file only.
for source in (surface, mesh, cut):
    Hide(source, view)

series = MetaFileSeriesReader(FileNames=sorted(glob.glob('spin_*.mhd')))
scene = GetAnimationScene()
scene.UpdateAnimationUsingDataTimeSteps()

spin = Contour(Input=series, ContourBy=['POINTS', array], Isosurfaces=[0.0])
Show(spin, view)
view.CameraPosition = [0, 0, 6]
view.CameraFocalPoint = [0, 0, 0]
view.CameraViewUp = [0, 1, 0]

times = series.TimestepValues
for label, t in (('first', times[0]), ('last', times[-1])):
    scene.AnimationTime = t
    Render(view)
    SaveScreenshot('spin_%s.png' % label, view, ImageResolution=[700, 700])
    print('saved spin_%s.png' % label)

#
# Renders the output of paraview_cantilever with ParaView, without opening a window:
#
#   pvbatch render.py
#
# Run it in the directory paraview_cantilever wrote its files to. It saves one picture per
# moment listed in MOMENTS below, as beam_<frame>.png. Nothing is exaggerated: soft rubber
# really droops this far.
#
from paraview.simple import *
import glob

SECONDS_PER_FRAME = 0.06
MOMENTS = [0, 10, 23, 40, 100]     # frames: released, falling, lowest point (1.38 s), rebound, settled
STRESS_RANGE = [0.0, 1.0e6]        # Pa, fixed so that frames can be compared

view = GetActiveViewOrCreate('RenderView')
view.ViewSize = [1400, 1000]
view.Background = [1, 1, 1]
try:
    view.UseColorPaletteForBackground = 0   # ParaView 5.10 and later
except AttributeError:
    pass
view.OrientationAxesVisibility = 0

# The beam at every frame, read as one time series. The legacy VTK reader groups numbered
# files the same way the MetaImage reader does.
files = sorted(glob.glob('beam_*.vtk'))
beam = LegacyVTKReader(FileNames=files)
scene = GetAnimationScene()
scene.UpdateAnimationUsingDataTimeSteps()

# Where the beam was at rest: the first frame's outline, drawn over every frame.
rest = LegacyVTKReader(FileNames=[files[0]])
rest_display = Show(rest, view)
rest_display.SetRepresentationType('Outline')
ColorBy(rest_display, None)
rest_display.AmbientColor = rest_display.DiffuseColor = [0.55, 0.55, 0.55]
rest_display.LineWidth = 1.5

# The wall the beam is clamped to, at x = 0.
wall = Box(XLength=0.1, YLength=3.8, ZLength=1.2, Center=[-0.05, -1.0, 0.25])
wall_display = Show(wall, view)               # a plain shape: solid colour already
wall_display.AmbientColor = wall_display.DiffuseColor = [0.35, 0.35, 0.38]

# The beam, coloured by von Mises stress: how hard each element of material is being worked.
beam_display = Show(beam, view)
beam_display.SetRepresentationType('Surface With Edges')
beam_display.EdgeColor = [0.25, 0.25, 0.3]
ColorBy(beam_display, ('CELLS', 'von_mises'))
lut = GetColorTransferFunction('von_mises')
lut.ApplyPreset('Blues', True)
lut.InvertTransferFunction()               # the preset runs dark-to-light; more stress should read darker
lut.RescaleTransferFunction(*STRESS_RANGE)
bar = GetScalarBar(lut, view)
bar.Title = 'von Mises stress (Pa)'
bar.ComponentTitle = ''
bar.TitleColor = bar.LabelColor = [0.1, 0.1, 0.1]
bar.RangeLabelFormat = '%.1e'
beam_display.SetScalarBarVisibility(view, True)

caption = Text()
caption_display = Show(caption, view)
caption_display.Color = [0.1, 0.1, 0.1]
caption_display.FontSize = 20
caption_display.WindowLocation = 'Upper Left Corner'

view.CameraPosition = [2.3, -0.8, 10.0]
view.CameraFocalPoint = [2.0, -1.05, 0.25]
view.CameraViewUp = [0, 1, 0]
view.CameraViewAngle = 30

for frame in MOMENTS:
    scene.AnimationTime = beam.TimestepValues[frame]
    caption.Text = 't = %.2f s' % (frame * SECONDS_PER_FRAME)
    Render(view)
    SaveScreenshot('beam_%04d.png' % frame, view, ImageResolution=[1400, 1000])
    print('saved beam_%04d.png' % frame)

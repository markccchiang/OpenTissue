#
# Renders the output of paraview_double_pendulum with ParaView, without opening a window:
#
#   pvbatch render.py
#
# Run it in the directory paraview_double_pendulum wrote its files to. It saves one picture
# per moment listed in MOMENTS below, as pendulum_<frame>.png.
#
# The pendulum drawn solid is OpenTissue's multibody engine; the faint one is the reference,
# the equations of motion integrated directly. They move together until chaos parts them.
#
from paraview.simple import *
import glob

SECONDS_PER_FRAME = 0.04
MOMENTS = [0, 25, 75, 150, 188, 250]   # frames: 0, 1, 3, 6, 7.52 and 10 seconds
ROD_RADIUS = 0.025
JOINT_RADIUS = 0.05

view = GetActiveViewOrCreate('RenderView')
view.ViewSize = [1200, 1000]
view.Background = [1, 1, 1]
try:
    view.UseColorPaletteForBackground = 0   # ParaView 5.10 and later
except AttributeError:
    pass
view.OrientationAxesVisibility = 0

def series(prefix):
    return LegacyVTKReader(FileNames=sorted(glob.glob(prefix + '_*.vtk')))

# The path of the tip so far, coloured by time.
trace = series('trace')
trace_display = Show(trace, view)
ColorBy(trace_display, ('POINTS', 'time'))
trace_lut = GetColorTransferFunction('time')
# The preset's name differs between ParaView versions.
for preset in ['Viridis (matplotlib)', 'Viridis']:
    try:
        trace_lut.ApplyPreset(preset, True)
        break
    except RuntimeError:
        pass
trace_lut.RescaleTransferFunction(0.0, 10.0)
trace_display.LineWidth = 1.5
bar = GetScalarBar(trace_lut, view)
bar.Title = 'time (s)'
bar.ComponentTitle = ''
bar.TitleColor = bar.LabelColor = [0.1, 0.1, 0.1]
trace_display.SetScalarBarVisibility(view, True)

# The reference pendulum: thin, grey, see-through.
reference = series('reference')
reference_display = Show(Tube(Input=reference, Radius=0.6 * ROD_RADIUS, NumberofSides=16), view)
reference_display.ColorArrayName = ['POINTS', '']   # a solid colour
reference_display.AmbientColor = reference_display.DiffuseColor = [0.5, 0.5, 0.5]
reference_display.Opacity = 0.35

# The engine's pendulum: rods as tubes, the pivot, elbow and tip as balls.
pendulum = series('pendulum')
rods_display = Show(Tube(Input=pendulum, Radius=ROD_RADIUS, NumberofSides=24), view)
rods_display.ColorArrayName = ['POINTS', '']   # a solid colour
rods_display.AmbientColor = rods_display.DiffuseColor = [0.85, 0.35, 0.1]
joints_display = Show(pendulum, view)
joints_display.SetRepresentationType('Point Gaussian')
joints_display.ShaderPreset = 'Sphere'
joints_display.GaussianRadius = JOINT_RADIUS
joints_display.ColorArrayName = ['POINTS', '']   # a solid colour
joints_display.AmbientColor = joints_display.DiffuseColor = [0.25, 0.25, 0.3]

scene = GetAnimationScene()
scene.UpdateAnimationUsingDataTimeSteps()

caption = Text()
caption_display = Show(caption, view)
caption_display.Color = [0.1, 0.1, 0.1]
caption_display.FontSize = 18
caption_display.WindowLocation = 'Upper Left Corner'
caption_display.Justification = 'Left'

# Straight on, looking along -z, with y up. Render once first: ParaView fits the camera to the
# data on the first render, which would undo these settings.
Render(view)
view.CameraPosition = [0.0, -0.4, 8.0]
view.CameraFocalPoint = [0.0, -0.4, 0.0]
view.CameraViewUp = [0, 1, 0]
view.CameraViewAngle = 30

for frame in MOMENTS:
    scene.AnimationTime = pendulum.TimestepValues[frame]
    caption.Text = 't = %.2f s\norange: OpenTissue multibody engine\ngrey: exact solution' % (frame * SECONDS_PER_FRAME)
    Render(view)
    SaveScreenshot('pendulum_%04d.png' % frame, view, ImageResolution=[1200, 1000])
    print('saved pendulum_%04d.png' % frame)

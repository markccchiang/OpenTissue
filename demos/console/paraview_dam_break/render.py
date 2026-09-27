#
# Renders the output of paraview_dam_break with ParaView, without opening a window:
#
#   pvbatch render.py
#
# Run it in the directory paraview_dam_break wrote its files to. It saves one picture per
# moment listed in MOMENTS below, as water_<frame>.png.
#
from paraview.simple import *
import glob

SECONDS_PER_FRAME = 0.02
MOMENTS = [0, 10, 20, 30, 50, 150]     # frames: 0, 0.2, 0.4, 0.6, 1.0 and 3.0 seconds
PARTICLE_RADIUS = 0.0136               # half the particle spacing, as in the program
SPEED_RANGE = [0.0, 2.5]               # m/s, fixed so that frames can be compared

view = GetActiveViewOrCreate('RenderView')
view.ViewSize = [1400, 900]
view.Background = [1, 1, 1]
try:
    view.UseColorPaletteForBackground = 0   # ParaView 5.10 and later
except AttributeError:
    pass
view.OrientationAxesVisibility = 0

# The tank: twelve edges, drawn as lines.
tank = LegacyVTKReader(FileNames=['tank.vtk'])
tank_display = Show(tank, view)          # no data arrays, so it is drawn in a solid colour
tank_display.AmbientColor = tank_display.DiffuseColor = [0.3, 0.3, 0.35]
tank_display.LineWidth = 2.0

# The water: the particles at every frame, read as one time series. Point Gaussian draws each
# particle as a small shaded sphere, far cheaper than a glyph per particle.
files = sorted(glob.glob('water_*.vtk'))
water = LegacyVTKReader(FileNames=files)
scene = GetAnimationScene()
scene.UpdateAnimationUsingDataTimeSteps()

water_display = Show(water, view)
water_display.SetRepresentationType('Point Gaussian')
water_display.ShaderPreset = 'Sphere'
water_display.GaussianRadius = PARTICLE_RADIUS
ColorBy(water_display, ('POINTS', 'speed'))
lut = GetColorTransferFunction('speed')
lut.ApplyPreset('Blues', True)
lut.InvertTransferFunction()           # so that still water is near white and fast water dark blue
lut.RescaleTransferFunction(*SPEED_RANGE)
bar = GetScalarBar(lut, view)
bar.Title = 'speed (m/s)'
bar.ComponentTitle = ''
bar.TitleColor = bar.LabelColor = [0.1, 0.1, 0.1]
water_display.SetScalarBarVisibility(view, True)

caption = Text()
caption_display = Show(caption, view)
caption_display.Color = [0.1, 0.1, 0.1]
caption_display.FontSize = 20
caption_display.WindowLocation = 'Upper Left Corner'

# Looking into the tank from the front, a little from above and to the side.
view.CameraPosition = [0.95, -1.35, 0.75]
view.CameraFocalPoint = [0.5, 0.1, 0.2]
view.CameraViewUp = [0, 0, 1]
view.CameraViewAngle = 30

for frame in MOMENTS:
    scene.AnimationTime = water.TimestepValues[frame]
    caption.Text = 't = %.2f s' % (frame * SECONDS_PER_FRAME)
    Render(view)
    SaveScreenshot('water_%04d.png' % frame, view, ImageResolution=[1400, 900])
    print('saved water_%04d.png' % frame)

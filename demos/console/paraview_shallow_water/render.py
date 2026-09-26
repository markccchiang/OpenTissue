#
# Renders the output of paraview_shallow_water with ParaView, without opening a window:
#
#   pvbatch render.py
#
# Run it in the directory paraview_shallow_water wrote its files to. It saves one picture per
# moment listed in MOMENTS below, as water_<frame>.png.
#
# The waves are a few hundredths of a unit high on a pool ten units across, so at true scale
# they would look flat. Like most pictures of water waves, these stretch the vertical axis --
# by VERTICAL_EXAGGERATION -- and every picture says so.
#
from paraview.simple import *
import glob

VERTICAL_EXAGGERATION = 3.0
REST_LEVEL = 1.0                           # the water level at rest, as in the program
SECONDS_PER_FRAME = 0.06
MOMENTS = [0, 8, 17, 33, 50, 100]          # frames: 0, 0.48, 1.02, 1.98, 3.0 and 6.0 seconds

view = GetActiveViewOrCreate('RenderView')
view.ViewSize = [1400, 900]
view.Background = [1, 1, 1]
try:
    view.UseColorPaletteForBackground = 0   # ParaView 5.10 and later
except AttributeError:
    pass
view.OrientationAxesVisibility = 0

# The sea bed. Both files hold "z minus height" in a two-layer grid, so the Contour at 0 is
# the surface itself.
bed = OpenDataFile('sea_bed.mhd')
array = bed.PointData.keys()[0]            # 'MetaImage'
bed_surface = Contour(Input=bed, ContourBy=['POINTS', array], Isosurfaces=[0.0])
bed_display = Show(bed_surface, view)
ColorBy(bed_display, None)                 # a plain colour: ParaView would otherwise colour it by the data
bed_display.AmbientColor = bed_display.DiffuseColor = [0.80, 0.72, 0.55]
bed_display.Scale = [1, 1, VERTICAL_EXAGGERATION]

# The water, one file per frame, read as a time series. They share one grid, which matters:
# ParaView takes a series' grid geometry from its first file only.
series = MetaFileSeriesReader(FileNames=sorted(glob.glob('water_*.mhd')))
scene = GetAnimationScene()
scene.UpdateAnimationUsingDataTimeSteps()
water = Contour(Input=series, ContourBy=['POINTS', array], Isosurfaces=[0.0])

# Colour the surface by how far it stands above (red) or below (blue) the water at rest.
elevation = Calculator(Input=water)
elevation.ResultArrayName = 'elevation'
elevation.Function = 'coordsZ - %g' % REST_LEVEL
water_display = Show(elevation, view)
water_display.Scale = [1, 1, VERTICAL_EXAGGERATION]
water_display.Opacity = 0.8
ColorBy(water_display, ('POINTS', 'elevation'))
lut = GetColorTransferFunction('elevation')
lut.ApplyPreset('Cool to Warm', True)
lut.RescaleTransferFunction(-0.05, 0.05)   # fixed, so frames can be compared; the drop saturates
bar = GetScalarBar(lut, view)
bar.Title = 'height above rest'
bar.ComponentTitle = ''
bar.TitleColor = bar.LabelColor = [0.1, 0.1, 0.1]
bar.RangeLabelFormat = '%.2f'
water_display.SetScalarBarVisibility(view, True)

caption = Text()
caption_display = Show(caption, view)
caption_display.Color = [0.1, 0.1, 0.1]
caption_display.FontSize = 20
caption_display.WindowLocation = 'Upper Left Corner'

view.CameraPosition = [-3.0, -6.0, 10.5]
view.CameraFocalPoint = [5.0, 5.0, 1.5]
view.CameraViewUp = [0, 0, 1]
view.CameraViewAngle = 30

for frame in MOMENTS:
    scene.AnimationTime = series.TimestepValues[frame]
    caption.Text = 't = %.2f s   (vertical scale x%g)' % (frame * SECONDS_PER_FRAME, VERTICAL_EXAGGERATION)
    Render(view)
    SaveScreenshot('water_%04d.png' % frame, view, ImageResolution=[1400, 900])
    print('saved water_%04d.png' % frame)

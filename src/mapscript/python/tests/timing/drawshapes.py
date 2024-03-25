# $Id$
#
# Timing tests of feature drawing -- map.draw vs drawing features
# shape by shape.

import getopt
import os
import sys
import timeit
from random import random

from .testing import mapscript

# Get Number of shapes from the command line
try:
    opts, args = getopt.getopt(sys.argv[1:], "n:")
except getopt.GetoptError:
    sys.exit(2)

numshapes = 100  # default to 100
for o, a in opts:
    if o == "-n":
        numshapes = int(a)

# The shapefileObj
shpfile = mapscript.shapefileObj("timing.shp", mapscript.MS_SHAPEFILE_POLYGON)

# Inline feature layer
ilayer = mapscript.layerObj()
ilayer.type = mapscript.MS_LAYER_POLYGON
ilayer.setProjection("init=epsg:4326")
ilayer.status = mapscript.MS_DEFAULT
ilayer.connectiontype = mapscript.MS_INLINE

print(numshapes, "shapes")

i = 0
while i < numshapes:
    # The shape to add is randomly generated
    xc = 4.0 * (random() - 0.5)
    yc = 4.0 * (random() - 0.5)
    r = mapscript.rectObj(xc - 0.25, yc - 0.25, xc + 0.25, yc + 0.25)
    s = r.toPolygon()

    # Add to shapefile
    shpfile.add(s)

    # Add to inline feature layer
    ilayer.addFeature(s)

    i = i + 1

del shpfile  # closes up the file

# Prepare the testing fixture
m = mapscript.mapObj("timing.map")
l = m.getLayerByName("POLYGON")
l.data = os.path.join(os.getcwd(), "timing")

# Save three map images to check afterwards
img = m.draw()
img.save("timing.png")

shpfile = mapscript.shapefileObj("timing.shp")
img = m.prepareImage()
for i in range(shpfile.numshapes):
    s = shpfile.getShape(i)
    s.classindex = 0
    s.draw(m, l, img)
img.save("timing-shapes.png")

class0 = mapscript.classObj(ilayer)
class0.insertStyle(l.getClass(0).getStyle(0))
img = m.prepareImage()
ilayer.draw(m, img)
img.save("timing-inline.png")

# =========================================================================
# Test 1A: Draw all shapes at once using map.draw()

print("Test 1A: draw map, all shapes at once")
s = """\
img = m.draw()
"""
t = timeit.Timer(stmt=s, setup="from __main__ import m")
print("%.2f usec/pass" % (1000000 * t.timeit(number=100) / 100))


# =========================================================================
# Test 1B: Draw shape by shape from the shapefileObj

print("Test 1B: draw shapes one at a time")
s = """\
img = m.prepareImage()
for i in range(shpfile.numshapes):
    s = shpfile.getShape(i)
    s.classindex = 0
    s.draw(m, l, img)
"""
t = timeit.Timer(stmt=s, setup="from __main__ import m, l, shpfile")
print("%.2f usec/pass" % (1000000 * t.timeit(number=100) / 100))


# =========================================================================
# Test 1C: Draw shapes after pushing them into an inline layer

print("Test 1C: draw inline layer shapes")
s = """\
img = m.prepareImage()
ilayer.draw(m, img)
"""
t = timeit.Timer(stmt=s, setup="from __main__ import m, ilayer")
print("%.2f usec/pass" % (1000000 * t.timeit(number=100) / 100))

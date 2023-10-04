# $Id$
#
# Timing tests of mapfile parsing vs map cloning

import os
import timeit
from shutil import copyfile

from .testing import TESTMAPFILE, mapscript

# ===========================================================================
# Test 1A: New maps from mapfile
#
# reloading the mapfile each time

print("Test 1A: reloading maps from mapfile")
s = """\
m = mapscript.mapObj(TESTMAPFILE)
"""
t = timeit.Timer(stmt=s, setup="from __main__ import mapscript, TESTMAPFILE")
print("%.2f usec/pass" % (1000000 * t.timeit(number=100) / 100))

# ===========================================================================
# Test 1B: Cloning
#
# Cloning instead of reloading

print("Test 1B: cloning maps instead of reloading")
m = mapscript.mapObj(TESTMAPFILE)
s = """\
c = m.clone()
"""
t = timeit.Timer(stmt=s, setup="from __main__ import m")
print("%.2f usec/pass" % (1000000 * t.timeit(number=100) / 100))

# ===========================================================================
# Test 2: Add 20 dups of the POLYGON layer to see how results scale

timing_map = mapscript.mapObj(TESTMAPFILE)
polygon_layer = timing_map.getLayerByName("POLYGON")

# duplicate POLYGON layer 20 times
for i in range(20):
    timing_map.insertLayer(polygon_layer)
assert timing_map.numlayers == 24

TIMINGMAPFILE = os.path.join(os.getcwd(), "timing.map")
timing_map.save(TIMINGMAPFILE)
copyfile("../../tests/fonts.txt", os.path.join(os.getcwd(), "fonts.txt"))
copyfile("../../tests/symbols.txt", os.path.join(os.getcwd(), "symbols.txt"))

# Test 2A: reloading mapfile
print("Test 2A: reloading inflated mapfile")
s = """\
m = mapscript.mapObj(TIMINGMAPFILE)
"""
t = timeit.Timer(stmt=s, setup="from __main__ import mapscript, TIMINGMAPFILE")
print("%.2f usec/pass" % (1000000 * t.timeit(number=100) / 100))

print("Test 2B: cloning inflated mapfile")
m = mapscript.mapObj(TIMINGMAPFILE)
s = """\
c = m.clone()
"""
t = timeit.Timer(stmt=s, setup="from __main__ import m")
print("%.2f usec/pass" % (1000000 * t.timeit(number=100) / 100))

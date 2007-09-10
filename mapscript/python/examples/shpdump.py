#!/usr/bin/env python

""" Dump the contents of the passed in shapefile.

    Pass in the basename of the shapefile.  This script uses the mapscript
    extention module.
"""

import mapscript
import sys
import os

# Utility functions.
def usage():
  """ Display usage if program is used incorrectly. """
  print "Syntax: %s base_filename" % sys.argv[0]
  sys.exit(2)

def plural(x):
  """ Returns an 's' if plural. 

  Useful in print statements to avoid something like 'point(s)'.  """
  if x > 1:
    return 's'
  return ''


# Make sure passing in filename argument.
if len(sys.argv) != 2:
  usage()

# Make sure can access .shp file, create shapefileObj.
if os.access(sys.argv[1] + ".shp", os.F_OK):
  sf_obj = mapscript.shapefileObj(sys.argv[1], -1)
else:
  print "Can't access shapefile"
  sys.exit(2)

# Create blank shape object.
s_obj = mapscript.shapeObj(-1)

# Loop through each shape in the shapefile.
for i in range(sf_obj.numshapes):

  # Get the ith object.
  sf_obj.get(i, s_obj)
  print "Shape %i has %i part%s." % (i, s_obj.numlines, plural(s_obj.numlines))
  print "bounds (%f, %f) (%f, %f)" % (s_obj.bounds.minx, s_obj.bounds.miny, s_obj.bounds.maxx, s_obj.bounds.maxy)

  # Loop through parts of each shape.
  for j in range(s_obj.numlines):

    # Get the jth part of the ith object.
    part = s_obj.get(j)
    print "Part %i has %i point%s." % (j, part.numpoints, plural(part.numpoints))

    # Loop through points in each part.
    for k in range(part.numpoints):

      # Get the kth point of the jth part of the ith shape.
      point = part.get(k)
      print "%i: %f, %f" % (k, point.x, point.y)

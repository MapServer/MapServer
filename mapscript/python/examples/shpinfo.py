#!/usr/bin/env python

""" Extracts basic descriptive information from the shapefile.

"""

import mapscript
import sys
import os

# Utility functions.
def usage():
  """ Display usage if program is used incorrectly. """
  print "Syntax: %s base_filename" % sys.argv[0]
  sys.exit(2)

# Make sure passing in filename argument.
if len(sys.argv) != 2:
  usage()

# Make sure can access .shp file, create shapefileObj.
if os.access(sys.argv[1] + ".shp", os.F_OK):
  sf_obj = mapscript.shapefileObj(sys.argv[1], -1)
else:
  print "Can't access shapefile"
  sys.exit(2)

# Dictionary of shapefile types.
types = { 1: 'point',
          3: 'arc',
          5: 'polygon',
          8: 'multipoint' }

# Print out basic information that is part of the shapefile object.
print "Shapefile %s:" % sys.argv[1]
print 
print "\ttype: %s" % types[sf_obj.type]
print "\tnumber of features: %i" % sf_obj.numshapes
print "\tbounds: (%f, %f) (%f, %f)" % (sf_obj.bounds.minx, 
                                       sf_obj.bounds.miny,
                                       sf_obj.bounds.maxx,
                                       sf_obj.bounds.maxy)

# Including a class to read DBF files.
# If not found, quit because can't do anything else without the dbfreader class.
try:
  import dbfreader
except:
  # Can't do anymore.
  sys.exit(2)

# Create DBF object.
dbf_obj = dbfreader.DBFFile(sys.argv[1] + ".dbf")

# Print out table characteristics.
print ""
print "\tDBF file: %s" % sys.argv[1] + ".dbf"
print "\tnumber of records: %i" % dbf_obj.get_record_count()
print "\tnumber of fields: %i" % len(dbf_obj.get_fields())
print ""
print "\t%-20s %12s %8s %10s" % ("Name", "Type",  "Length", "Decimals")
print "\t-----------------------------------------------------"

# Print out field characteristics.
for field in dbf_obj.get_fields():
  print "\t%-20s %12s %8d %10d" % (field.get_name(), field.get_type_name(), field.get_len(), field.field_places)


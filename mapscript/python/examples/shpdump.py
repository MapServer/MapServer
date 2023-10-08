#!/usr/bin/env python

"""
Dump the contents of the passed in Shapefile

Usage:

python shpdump.py polygon.shp

"""

import os
import sys

import mapscript


def plural(x):
    """
    Returns an 's' if plural

    Useful in print statements to avoid something like 'point(s)'
    """
    if x > 1:
        return "s"
    return ""


def get_shapefile_object(sf_path):

    # make sure can access .shp file, create shapefileObj

    if os.access(sf_path, os.F_OK):
        sf_obj = mapscript.shapefileObj(sf_path, -1)
    else:
        print("Can't access {}".format(sf_path))
        sys.exit(2)

    return sf_obj


def main(sf_path):

    if not sf_path.lower().endswith(".shp"):
        sf_path += ".shp"

    sf_obj = get_shapefile_object(sf_path)

    # create an empty Shapefile object

    s_obj = mapscript.shapeObj()

    # loop through each shape in the original Shapefile

    for i in range(sf_obj.numshapes):

        # get the object at index i
        sf_obj.get(i, s_obj)
        print("Shape %i has %i part%s" % (i, s_obj.numlines, plural(s_obj.numlines)))
        print(
            "Bounds (%f, %f) (%f, %f)"
            % (
                s_obj.bounds.minx,
                s_obj.bounds.miny,
                s_obj.bounds.maxx,
                s_obj.bounds.maxy,
            )
        )

        # loop through parts of each shape

        for j in range(s_obj.numlines):

            # get the jth part of the ith object

            part = s_obj.get(j)
            print(
                "Part %i has %i point%s" % (j, part.numpoints, plural(part.numpoints))
            )

            # loop through points in each part

            for k in range(part.numpoints):

                # get the kth point of the jth part of the ith shape

                point = part.get(k)
                print("%i: %f, %f" % (k, point.x, point.y))


def usage():
    """
    Display usage if program is used incorrectly
    """
    print("Syntax: %s <shapefile_path>" % sys.argv[0])
    sys.exit(2)


# make sure passing in filename argument
if len(sys.argv) != 2:
    usage()


sf_path = sys.argv[1]
main(sf_path)

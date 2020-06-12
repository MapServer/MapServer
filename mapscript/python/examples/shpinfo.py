#!/usr/bin/env python

"""
Extracts basic descriptive information from a Shapefile

Usage:

python shpinfo.py point.shp

"""
import mapscript
import sys
import os


def get_shapefile_object(sf_path):

    # make sure can access .shp file, create shapefileObj

    if os.access(sf_path, os.F_OK):
        sf_obj = mapscript.shapefileObj(sf_path)
    else:
        print("Can't access {}".format(sf_path))
        sys.exit(2)

    return sf_obj


def get_shapefile_details(sf_obj):

    # dictionary of shapefile types

    types = {
              mapscript.MS_SHAPEFILE_POINT: 'Point',
              mapscript.MS_SHAPEFILE_ARC: 'Line',
              mapscript.MS_SHAPEFILE_POLYGON: 'Polygon',
              mapscript.MS_SHAPEFILE_MULTIPOINT: 'Multipoint',

              mapscript.MS_SHP_POINTZ: 'PointZ',
              mapscript.MS_SHP_ARCZ: 'LineZ',
              mapscript.MS_SHP_POLYGONZ: 'PolygonZ',
              mapscript.MS_SHP_MULTIPOINTZ: 'MultipointZ',

              mapscript.MS_SHP_POINTM: 'Multipoint',
              mapscript.MS_SHP_ARCM: 'LineM',
              mapscript.MS_SHP_POLYGONM: 'PolygonM',
              mapscript.MS_SHP_MULTIPOINTM: 'MultipointM'
            }

    # print out basic information that is part of the shapefile object

    print("\tType: %s" % types[sf_obj.type])

    print("\tBounds: (%f, %f) (%f, %f)" % (sf_obj.bounds.minx,
                                           sf_obj.bounds.miny,
                                           sf_obj.bounds.maxx,
                                           sf_obj.bounds.maxy))

    print("\tNumber of features: %i" % sf_obj.numshapes)


def get_dbf_details(sf_obj):

    # get DBF object

    dbf_obj = sf_obj.getDBF()

    # print out table characteristics

    print("\tNumber of records in DBF: %i" % dbf_obj.nRecords)
    print("\tNumber of fields: %i" % dbf_obj.nFields)
    print("")
    print("\t%-20s %12s %8s %10s" % ("Name", "Type",  "Length", "Decimals"))
    print("\t-----------------------------------------------------")

    # print out field characteristics

    for idx in range(0, dbf_obj.nFields):
        print("\t%-20s %12s %8d %10d" % (dbf_obj.getFieldName(idx), dbf_obj.getFieldType(idx),
                                         dbf_obj.getFieldWidth(idx), dbf_obj.getFieldDecimals(idx)))


def main(sf_path):

    if not sf_path.lower().endswith(".shp"):
        sf_path += ".shp"

    sf_obj = get_shapefile_object(sf_path)

    print("Shapefile %s:" % sf_path)
    print("")

    get_shapefile_details(sf_obj)
    get_dbf_details(sf_obj)


def usage():
    """
    Display usage if program is used incorrectly
    """
    print("Syntax: %s <shapefile_path>" % sys.argv[0])
    sys.exit(2)


# make sure a filename argument is provided
if len(sys.argv) != 2:
    usage()

main(sys.argv[1])

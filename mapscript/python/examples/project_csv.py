#!/usr/bin/python
#
# Script : project_csv.py
#
# Purpose: Simple example to read a csv file and reproject point x/y data
#
# $Id$
#

import sys
import csv
import mapscript

# example invocation
# ./reproj.py ./foo.csv 2 3 EPSG:32619 EPSG:4326 

# check input parameters
if (len(sys.argv) != 6):
    print sys.argv[0] + \
        " <csvfile> <x_col> <y_col> <epsg_code_in> <epsg_code_out>"
    sys.exit(1)
else:     
    # set x and y indices
    x = int(sys.argv[2])
    y = int(sys.argv[3])

    # set input and output projections
    projObjIn  = mapscript.projectionObj("init="+sys.argv[4].lower())
    projObjOut = mapscript.projectionObj("init="+sys.argv[5].lower())

    # open file
    fCsv = open(sys.argv[1], 'r')

    # read csv 
    csvIn  = csv.reader(fCsv)
    # setup output 
    csvOut = csv.writer(sys.stdout)

    for aRow in csvIn: # each record
        # set pointObj
        point = mapscript.pointObj(float(aRow[x]), float(aRow[y]))
        # project
        point.project(projObjIn, projObjOut)

        # update with reprojected coordinates
        aRow[x] = point.x
        aRow[y] = point.y

        csvOut.writerow(aRow)
    fCsv.close()

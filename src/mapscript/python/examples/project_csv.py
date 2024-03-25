#!/usr/bin/env python

"""
Simple example to read a csv file and reproject point x/y data

Usage:

project_csv.py cities.csv 2 1 EPSG:4326 EPSG:3857

"""

import csv
import sys
from io import open

import mapscript


def main(input_file, x_field_idx, y_field_idx, input_proj, output_proj):

    # set input and output projections
    proj_in = mapscript.projectionObj(input_proj)
    proj_out = mapscript.projectionObj(output_proj)

    # open file
    with open(input_file, encoding="utf-8") as f:
        # read csv
        csv_in = csv.reader(f)
        headers = next(csv_in)

        # setup output
        csv_out = csv.writer(sys.stdout)
        csv_out.writerow(headers)

        for row in csv_in:
            # set pointObj
            point = mapscript.pointObj(float(row[x_field_idx]), float(row[y_field_idx]))
            # project
            point.project(proj_in, proj_out)

            # update with reprojected coordinates
            row[x_field_idx] = point.x
            row[y_field_idx] = point.y

            csv_out.writerow(row)


def usage():
    """
    Display usage if program is used incorrectly
    """
    print(
        "Syntax: %s <csvfile> <x_col> <y_col> <epsg_code_in> <epsg_code_out>"
        % sys.argv[0]
    )
    sys.exit(2)


# check input parameters

if len(sys.argv) != 6:
    usage()


input_file = sys.argv[1]

# set x and y indices

x_field_idx = int(sys.argv[2])
y_field_idx = int(sys.argv[3])

# get projection codes

input_proj = "init=" + sys.argv[4].lower()
output_proj = "init=" + sys.argv[5].lower()
main(input_file, x_field_idx, y_field_idx, input_proj, output_proj)

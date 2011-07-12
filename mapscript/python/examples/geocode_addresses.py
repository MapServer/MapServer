#!/usr/bin/python

# Script : geocode_addresses.py
#
# Purpose: simple script to read a csv file, geocode the addresses
# with the Google Maps API and write to a new shapefile
#
# $Id$
#

import sys
import csv
import mapscript
import urllib
import urllib2
from lxml import etree

# uses dbfpy http://pypi.python.org/pypi/dbfpy/2.2.1
from dbfpy.dbf import *

# example invocation
# ./reproj.py ./foo.csv address_colnum, city_colnum, stateprov_colnum ./bar.csv

# check input parameters
if (len(sys.argv) != 6):
    print sys.argv[0] + \
    " <csvfile> <address_colnum> <city_colnum> <stateprov_colnum> <outfile>"
    sys.exit(1)

# geocoder base request URL
sGeocoderUrl = "http://www.geocoder.ca/?geoit=xml&locate="

# set csv record indices
sAddress   = int(sys.argv[2])
sCity      = int(sys.argv[3])
sStateProv = int(sys.argv[4])

# open file
fCsv = open(sys.argv[1], 'r')

# read csv 
csvIn = csv.reader(fCsv)

# create output shp/shx
msSFOut = mapscript.shapefileObj(sys.argv[-1], 1)

# create output dbf
dbfOut = Dbf(sys.argv[-1]+".dbf", new=True)

# add fields
dbfOut.addField(
    ("address", 'C', 255),
    ("city", 'C', 255),
    ("stateprov", 'C', 255),
    ("x", 'N', 6,2),
    ("y", 'N', 7,2)
)

for aRow in csvIn:
    # concatenate request params
    sRequest = aRow[sAddress] + "," + aRow[sCity] + "," + aRow[sStateProv]

    # contatenate request params (escaped) and base URL  
    sRequest = sGeocoderUrl + urllib.quote(sRequest)

    # make the HTTP request
    ul2Response = urllib2.urlopen(sRequest)

    # serialize into etree XML object
    etTree = etree.parse(ul2Response)
    sY = float(etTree.find('latt').text)
    sX = float(etTree.find('longt').text)

    # serialize mapscript pointObj and add to shapefile
    msPoint = mapscript.pointObj(sX, sY)
    msSFOut.addPoint(msPoint)

    # add dbf record
    dRec=dbfOut.newRecord()
    dRec['address']   = aRow[sAddress]
    dRec['city']      = aRow[sCity]
    dRec['StateProv'] = aRow[sStateProv]
    dRec['x']         = sX
    dRec['y']         = sY
    dRec.store()
# close files
fCsv.close()
dbfOut.close()

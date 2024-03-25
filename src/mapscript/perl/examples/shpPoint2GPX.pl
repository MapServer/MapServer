#!/usr/bin/perl
#
# Script : shpPoint2GPX.pl
#
# Purpose: Converts a Point shapefile to a GPX document
#          as per http://www.topografix.com/gpx.asp
#
# $Id$
#

use strict;
use warnings;
use POSIX;
use XBase;
use mapscript;
use Getopt::Long;

my ($infile, $outfile, $namecol);

GetOptions("input=s", \$infile, "output=s", \$outfile, "namecol=s", \$namecol);

if(!$infile or !$outfile or !$namecol) {
  print "Usage: $0 --input=[filename] --output=[filename] --namecol=[namecol]\n"; 
  exit 0;
}

# open the input shapefile and dbf
my $shapefile = new mapscript::shapefileObj($infile, -1) or die "Unable to open shapefile $infile: $!\n";
my $table     = new XBase "$infile" or die XBase->errstr;

if ($shapefile->{type} != 1) {
  print "input shapefile must be of type point\n";
  exit 0;
}

# unlink and create the output GPX document
unlink "$outfile.gpx";

open(GPX, ">$outfile.gpx") or die "Unable to open GPX document $outfile.gpx: $!\n";

# print the GPX header info
print <<END;
<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<gpx creator="shpPoint2GPX.pl" version="1.1" xsi:schemaLocation="http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd" xmlns="http://www.topografix.com/GPX/1/1" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:ms="http://mapserver.gis.umn.edu/mapserver">
	<metadata>
		<name>$outfile.gpx</name>
		<desc>Description</desc>
		<author>
			<name>Name</name>
			<email id="foouser" domain="mapserver.gis.umn.edu"/>
			<link href="http://mapserver.gis.unm.edu/">
				<text>Welcome to MapServer</text>
				<type>text/html</type>
			</link>
		</author>
		<copyright author="Regents of the University of Minnesota">
			<year>2007</year>
			<license>http://mapserver.gis.umn.edu/License</license>
		</copyright>
		<link href="http://mapserver.gis.unm.edu/">
			<text>Welcome to MapServer</text>
			<type>text/html</type>
		</link>
		<time>2001-12-17T09:30:47.0Z</time>
		<keywords>MapServer, mapscript, perl, GPX, GPS</keywords>
		<bounds maxlat="$shapefile->{bounds}->{maxx}" maxlon="$shapefile->{bounds}->{maxy}" minlat="$shapefile->{bounds}->{miny}" minlon="$shapefile->{bounds}->{minx}"/>
		<extensions/>
	</metadata>
END

my $shape = new mapscript::shapeObj(-1); # something to hold shapes

# print each shape as a waypoint
for(my $i=0; $i<$shapefile->{numshapes}; $i++) {
  $shapefile->get($i, $shape);

  for(my $j=0; $j<$shape->{numlines}; $j++) {
    my $part = $shape->get($j);
    for(my $k=0; $k<$part->{numpoints}; $k++) {
      my $point = $part->get($k);
      my %row = $table->get_record_as_hash($j) or die $table->errstr;
      delete $row{"_DELETED"};
      print "\t<wpt lat=\"$point->{y}\" lon=\"$point->{x}\">\n";

      if ($row{uc($namecol)}) {
        print "\t\t<name>$row{$namecol}</name>\n";
      }

      # print out the dbf values as GPX extensions
      print "\t\t<extensions>\n";

      foreach my $k (keys %row) {
        print "\t\t\t<ms:$k><![CDATA[$row{$k}]]></ms:$k>\n";
      }
      print "\t\t</extensions>\n\t</wpt>\n";
    }
  }
} # for each shape

print <<END;
	<extensions>
		<ms:numshapes>$shapefile->{numshapes}</ms:numshapes>
		<ms:source>$shapefile->{source}</ms:source>
	</extensions>
</gpx>
END

undef $shapefile;
close(GPX);

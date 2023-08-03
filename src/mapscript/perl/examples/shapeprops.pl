#!/usr/bin/perl
#
# Script : shapeprops.pl
#
# Purpose: Applies area, perimeter and centroid calculations to a shapes
#          in a shapefile (requires mapserver/mapscript to be built with GEOS)
#
# $Id$
#

use strict;
use warnings;
use mapscript;

@ARGV == 1 or die "Usage: $0 <shapefile>\n";

# open the shapefile
my $sf = new mapscript::shapefileObj($ARGV[0], -1) or die "Unable to open shapefile $ARGV[0]: $!\n";

# loop over every shape
for(my $i = 0; $i < $sf->{numshapes}; $i++) {
  # fetch the shape
  my $s = $sf->getShape($i);

  # calculate area
  my $a = $s->getArea();

  # calculate length / perimeter
  my $l = $s->getLength();

  # calculate centroid
  my $c = $s->getCentroid();

  print <<END;

Area of shape $i: $a
Length / Perimeter of shape $i: $l
Centroid of shape $i: $c->{x}, $c->{y}
END

  # free shape
  undef $s;
}

# free shapefile
undef $sf;

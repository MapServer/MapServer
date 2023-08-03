#!/usr/bin/perl
#
# Script : distanceToPoint.pl
#
# Purpose: Returns distance between a shape and a point
#
# $Id$
#

use strict;
use warnings;
use POSIX;
use XBase;
use mapscript;
use Getopt::Long;
use File::Copy;

my ($infile, $infile_shpid, $x, $y, $distance);

GetOptions("infile=s", \$infile, "infile_shpid=s", \$infile_shpid, "x=s", \$x, "y=s", \$y);

if(!$infile or !$infile_shpid or !$x or !$y) {
  print "Usage: $0 --infile=[filename] --infile_shpid=[shpid] --x=[x] --y=[y]\n";
  exit 0;
}

# open the first input shapefile
my $inshpf = new mapscript::shapefileObj($infile, -1) or die "Unable to open shapefile $infile.";

my $inshape = $inshpf->getShape($infile_shpid);

my $inpt = new mapscript::pointObj($x, $y);

$distance = $inshape->distanceToPoint($inpt);

undef $inshpf;

print "Distance between shape $infile/$infile_shpid and point $x $y is $distance\n";


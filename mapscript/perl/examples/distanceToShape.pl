#!/usr/bin/perl
#
# Script : distanceToShape.pl
#
# Purpose: Returns distance between two shapes
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

my ($infile1, $infile1_shpid, $infile2, $infile2_shpid, $distance);

GetOptions("infile1=s", \$infile1, "infile1_shpid=s", \$infile1_shpid, "infile2=s", \$infile2, "infile2_shpid=s", \$infile2_shpid);

if(!$infile1 or !$infile1_shpid or !$infile2 or !$infile2_shpid) {
  print "Usage: $0 --infile1=[filename] --infile1_shpid=[shpid] --infile2=[filename] --infile2_shpid=[shpid]\n";
  exit 0;
}

# open the first input shapefile
my $inshpf1 = new mapscript::shapefileObj($infile1, -1) or die "Unable to open shapefile $infile1.";
my $inshpf2 = new mapscript::shapefileObj($infile2, -1) or die "Unable to open shapefile $infile2.";

my $inshape1 = $inshpf1->getShape($infile1_shpid);
my $inshape2 = $inshpf2->getShape($infile2_shpid);

$distance = $inshape1->distanceToShape($inshape2);

undef $inshpf1;
undef $inshpf2;

print "Distance between shape $infile1/$infile1_shpid and shape $infile2/$infile2_shpid is $distance units\n";


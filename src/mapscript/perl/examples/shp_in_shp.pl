#!/usr/bin/perl
#
# Script : shp_in_shp.pl
#
# Purpose: Tests whether a shape is within another shape
#
# $Id$
#

use strict;
use warnings;
use mapscript;
use Getopt::Long;
use File::Copy;

my ($infile1, $infile1_shpid, $infile2, $infile2_shpid, $within);

GetOptions("infile1=s", \$infile1, "infile1_shpid=s", \$infile1_shpid, "infile2=s", \$infile2, "infile2_shpid=s", \$infile2_shpid);

# shpid can be zero, which looks false, so use defined()
if(!$infile1 or !defined($infile1_shpid) or !$infile2 or !defined($infile2_shpid)) {
  print "Usage: $0 --infile1=[filename] --infile1_shpid=[shpid] --infile2=[filename] --infile2_shpid=[shpid]\n";
  exit 0;
}

# open the first input shapefile
my $inshpf1 = new mapscript::shapefileObj($infile1, -1) or die "Unable to open shapefile $infile1.";
my $inshpf2 = new mapscript::shapefileObj($infile2, -1) or die "Unable to open shapefile $infile2.";

my $inshape1 = $inshpf1->getShape($infile1_shpid);
my $inshape2 = $inshpf2->getShape($infile2_shpid);

$within = $inshape1->within($inshape2);

if ($within == 1) {
  $within = "WITHIN";
}
elsif ($within == 0) {
  $within = "NOT WITHIN";
}

undef $inshpf1;
undef $inshpf2;

print "Shape $infile1/$infile1_shpid is $within shape $infile2/$infile2_shpid\n";


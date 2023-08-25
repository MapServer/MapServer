#!/usr/bin/perl
#
# Script : buffer.pl
#
# Purpose: Applies buffer to shapefile dataset using geos support
#          buffer units as are per units of data
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

my ($infile, $outfile, $buffer);

GetOptions("input=s", \$infile, "output=s", \$outfile, "buffer=n", \$buffer);

if(!$infile or !$outfile or !$buffer) {
  print "Usage: $0 --input=[filename] --output=[filename] --buffer=[native units]\n";
  exit 0;
}

die "Tolerance must be greater than zero." unless $buffer > 0;

# initialize counters for reporting
my $incount  = 0;
my $outcount = 0;

# open the input shapefile
my $inSHP = new mapscript::shapefileObj($infile, -1) or die "Unable to open shapefile $infile.";

# create the output shapefile

unlink "$outfile.shp";
unlink "$outfile.shx";
unlink "$outfile.dbf";

my $outSHP = new mapscript::shapefileObj($outfile, $inSHP->{type}) or die "Unable to create shapefile '$outfile'. $!\n";

copy("$infile.dbf", "$outfile.dbf") or die "Can't copy file $infile.dbf to $outfile.dbf: $!\n";

my $inshape = new mapscript::shapeObj(-1); # something to hold shapes

for(my $i=0; $i<$inSHP->{numshapes}; $i++) {
  $inSHP->get($i, $inshape);
  my $outshape = new mapscript::shapeObj(-1);

  print "buffering feature: $i\n";

  $outshape = $inshape->buffer($buffer) or die "Unable to buffer feature #$i: $!\n"; # in native map units

  $outSHP->add($outshape);
  undef($outshape); # free memory associated with shape

} # for each shape

$outSHP = ''; # write the file

undef $inSHP;
undef $outSHP;


#!/usr/bin/perl
# Script : thin.pl
#
# Purpose: An adaption of the ArcView Avenue example script genfeat.ave. It's
#          basically the Douglas-Peucker generalization algorithm.
#          (http://mapserver.gis.umn.edu/community/scripts/thin.pl)
#
# $Id$
#

use strict;
use warnings;
use mapscript;
use Getopt::Long;
use File::Copy;

my ($infile, $outfile, $tolerance);

GetOptions("input=s", \$infile, "output=s", \$outfile, "tolerance=n", \$tolerance);

if(!$infile or !$outfile or !$tolerance) {
  print "Usage: $0 --input=[filename] --output=[filename] --tolerance=[maximum distance between vertices]\n";
  exit 0;
}

die "Tolerance must be greater than zero." unless $tolerance > 0;

# initialize counters for reporting
my $incount  = 0;
my $outcount = 0;

# open the input shapefile
my $inSHP = new mapscript::shapefileObj($infile, -1) or die "Unable to open shapefile $infile.";

die "Cannot thin point/multipoint shapefiles." unless ($inSHP->{type} == 5 or $inSHP->{type} == 3);

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

  for(my $j=0; $j<$inshape->{numlines}; $j++) {

    my $inpart  = $inshape->get($j);
    my $outpart = new mapscript::lineObj();

    my @stack = ();

    $incount += $inpart->{numpoints};

    my $anchor = $inpart->get(0); # save first point
    $outpart->add($anchor);
    my $aIndex = 0;

    my $fIndex = $inpart->{numpoints} - 1;
    push @stack, $fIndex;

    # Douglas - Peucker algorithm
    while(@stack) {

      $fIndex = $stack[$#stack];
      my $fPoint = $inpart->get($fIndex);

      my $max = $tolerance; # comparison values
      my $maxIndex = 0;

      # process middle points
      for (($aIndex+1) .. ($fIndex-1)) {

        my $point = $inpart->get($_);
        #my $dist = $point->distanceToLine($anchor, $fPoint);
        my $dist = $point->distanceToSegment($anchor, $fPoint);

        if($dist >= $max) {
          $max = $dist;
          $maxIndex = $_;
        }
      }

      if($maxIndex > 0) {
        push @stack, $maxIndex;
      } else {
        $outpart->add($fPoint);
        $anchor = $inpart->get(pop @stack);
        $aIndex = $fIndex;
      }
    }

    # check for collapsed polygons, use original data in that case
    if(($outpart->{numpoints} < 4) and ($inSHP->{type} == 5)) {
      $outpart = $inpart;
    }

    $outcount += $outpart->{numpoints};
    $outshape->add($outpart);

  } # for each part

  $outSHP->add($outshape);
  undef($outshape); # free memory associated with shape

} # for each shape

$outSHP = ''; # write the file

undef $inSHP;
undef $outSHP;

my $reduction = ((($outcount / $incount) * 100) - 100) * -1;

print <<END;

$0 Summary:

Input File  : $infile  ($incount vertices)
Output File : $outfile ($outcount vertices)
Tolerance   : $tolerance
Reduction   : $reduction%

END

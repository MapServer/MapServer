#!/usr/bin/perl

use strict;
use warnings;
use mapscript;
use Getopt::Long;

my $file;
my %types = ( '1' => 'point',
	      '3' => 'arc',
	      '5' => 'polygon',
	      '8' => 'multipoint'
	    );

GetOptions("file=s", \$file);

if(!$file) {
  print "Syntax: dump.pl --file=[filename]\n";
  exit 0;
}

my $shapefile = new mapscript::shapefileObj($file, -1) or die "Unable to open shapefile $file";

print "Shapefile opened (type=". $types{$shapefile->{type}} .") with ".
$shapefile->{numshapes} ." shape(s)\n";

my $shape = new mapscript::shapeObj(-1);

for(my $i=0; $i<$shapefile->{numshapes}; $i++) {
    
    $shapefile->get($i, $shape);

    print "Shape $i has ". $shape->{numlines} ." part(s) - ";
    printf "bounds (%f,%f) (%f,%f)\n", $shape->{bounds}->{minx}, $shape->{bounds}->{miny}, $shape->{bounds}->{maxx}, $shape->{bounds}->{maxy};

    for(my $j=0; $j<$shape->{numlines}; $j++) {
        my $part = $shape->get($j);
        print "Part $j has ". $part->{numpoints} ." point(s)\n";

        for(my $k=0; $k<$part->{numpoints}; $k++) {
            my $point = $part->get($k);
            print "$k: ". $point->{x} .", ". $point->{y} ."\n";
        }
    }
}

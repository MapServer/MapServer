#!/usr/bin/perl

use mapscript;
use Getopt::Long;

%types = ( '1' => 'point',
	   '3' => 'arc',
	   '5' => 'polygon',
	   '8' => 'multipoint'
	 );

&GetOptions("file=s", \$file);
if(!$file) {
  print "Syntax: dump.pl -file=[filename]\n";
  exit 0;
}

$shapefile = new shapefileObj($file, -1) or die "Unable to open shapefile $file";

print "Shapefile opened (type=". $types{$shapefile->{type}} .") with ".
$shapefile->{numshapes} ." shape(s)\n";

$shape = new shapeObj(-1);

for($i=0; $i<$shapefile->{numshapes}; $i++) {
    
    $shapefile->get($i, $shape);

    print "Shape $i has ". $shape->{numlines} ." part(s) - ";
    printf "bounds (%f,%f) (%f,%f)\n", $shape->{bounds}->{minx}, $shape->{bounds}->{miny}, $shape->{bounds}->{maxx}, $shape->{bounds}->{maxy};

    for($j=0; $j<$shape->{numlines}; $j++) {
        $part = $shape->get($j);
        print "Part $j has ". $part->{numpoints} ." point(s)\n";

        for($k=0; $k<$part->{numpoints}; $k++) {
            $point = $part->get($k);
            print "$k: ". $point->{x} .", ". $point->{y} ."\n";
        }
    }
}

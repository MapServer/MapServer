#!/usr/bin/perl

use strict;
use warnings;
use XBase;
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
  print "Syntax: shpinfo.pl --file=[filename]\n";
  exit 0;
}

my $shapefile = new mapscript::shapefileObj($file, -1) or die "Unable to open shapefile $file.";

print "Shapefile $file:\n\n";
print "\ttype: ". $types{$shapefile->{type}} ."\n"; 
print "\tnumber of features: ". $shapefile->{numshapes} ."\n";
printf "\tbounds: (%f,%f) (%f,%f)\n", $shapefile->{bounds}->{minx}, $shapefile->{bounds}->{miny}, $shapefile->{bounds}->{maxx}, $shapefile->{bounds}->{maxy};

my $table = new XBase $file.'.dbf' or die XBase->errstr;

print "\nXbase table $file.dbf:\n\n";

print "\tnumber of records: ". ($table->last_record+1) ."\n";
print "\tnumber of fields: ". ($table->last_field+1) ."\n\n";

print "\tName             Type Length Decimals\n";
print "\t---------------- ---- ------ --------\n";
foreach ($table->field_names) {
  printf "\t%-16s %4s %6d %8d\n", $_, $table->field_type($_), $table->field_length($_), $table->field_decimal($_)
}

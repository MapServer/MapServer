#!/usr/bin/perl
use strict;
use warnings;
use mapscript;
use Data::Dumper;

my $file=$ARGV[0];

# utility
sub assertNotNull {
	my ($o, $test) = @_;
	if ($o) {
		print $test . "  PASSED\n";
	} else {
		print $test . "  FAILED\n";
	}
}


# layerObj
sub testGetLayerObj {
	my $map = new mapscript::mapObj($file);
	my $layer = $map->getLayer(1);
	$map = undef;
	assertNotNull( $layer->{map} , "testGetLayerObj");
	#$layer->{map}->draw()->save("/tmp/map.png");
}

sub testGetLayerObjByName {
	my $map = new mapscript::mapObj($file);
	my $layer = $map->getLayerByName("POLYGON");
	$map = undef;
	assertNotNull( $layer->{map} , "testGetLayerObjByName");
}

sub testLayerObj {
	my $map = new mapscript::mapObj($file);
	my $layer = new mapscript::layerObj($map);
	$map = undef;
	assertNotNull( $layer->{map} , "testLayerObj");
}

sub testInsertLayerObj {
	my $map = new mapscript::mapObj($file);
	my $layer = new mapscript::layerObj(undef);
	my $position = $map->insertLayer($layer);
	$map = undef;
	assertNotNull( $position == 7 , "testInsertLayerObj position");
	assertNotNull( $layer->{map} , "testInsertLayerObj notnull");
}

# classObj
sub testGetClassObj {
	#dumpHash(mapscript::getPARENT_PTRS());
	my $map = new mapscript::mapObj($file);
	my $layer = $map->getLayer(1);
	my $clazz = $layer->getClass(0);
	#dumpHash(mapscript::getPARENT_PTRS());
	#$clazz->{layer}->{map}->draw()->save("/tmp/map1.png");
	#print "parent layer=".$clazz->{layer}."\n";
	$map = undef; $layer=undef;
	#print "parent layer=".$clazz->{layer}."\n";
	assertNotNull( $clazz->{layer} , "testGetClassObj");
	#$clazz->{layer}->{map}->draw()->save("/tmp/map2.png");
	#dumpHash(mapscript::getPARENT_PTRS());
}

sub testClassObj {
	my $map = new mapscript::mapObj($file);
	my $layer = $map->getLayer(1);
	my $clazz = new mapscript::classObj($layer);
	$map = undef; $layer=undef;
	assertNotNull( $clazz->{layer} , "testClassObj");
}

sub testInsertClassObj {
	my $map = new mapscript::mapObj($file);
	my $layer = $map->getLayer(1);
	my $clazz = new mapscript::classObj(undef);
	my $position = $layer->insertClass($clazz);
	$map = undef; $layer=undef;
	assertNotNull( $position == 2 , "testInsertClassObj position");
	assertNotNull( $clazz->{layer} , "testInsertClassObj notnull");
}

if ( ! $file ) {
	print "Usage: RFC24.pl file.map\n";
	exit 1;
}

testGetLayerObj;
testGetLayerObjByName;
testLayerObj;
testInsertLayerObj;
testGetClassObj;
testClassObj;
testInsertClassObj;

my $hashmap = mapscript::getPARENT_PTRS();
assertNotNull( keys( %$hashmap )==0 , "checking that hashmap of parent ptrs is empty");
print "No of keys:".keys( %$hashmap )."\n";
#dumpHash($hashmap);
print Dumper( $hashmap );


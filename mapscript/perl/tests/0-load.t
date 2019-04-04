#!/usr/bin/perl
use strict;
use warnings;
use Test::More tests => 11;
use File::Spec;

sub msHas
{
	my ($mod) = @_;
	my $ver = mapscript::msGetVersion();
	if (index($ver, $mod) > -1) {
		return 1;
	} else {
		return 0;
	}
}

require_ok( 'mapscript' );
diag("Testing: " . $INC{'mapscript.pm'});
diag( mapscript::msGetVersion() );

SKIP: {
	skip "no geos support", 10 unless msHas('SUPPORTS=GEOS');
    my @wkt_list = (
		'POINT (5.0000000000000000 7.0000000000000000)',
		'LINESTRING (5.0000000000000000 7.0000000000000000, 7.0000000000000000 9.0000000000000000, 9.0000000000000000 -1.0000000000000000)',
		'POLYGON ((500.0000000000000000 500.0000000000000000, 3500.0000000000000000 500.0000000000000000, 3500.0000000000000000 2500.0000000000000000, 500.0000000000000000 2500.0000000000000000, 500.0000000000000000 500.0000000000000000), (1000.0000000000000000 1000.0000000000000000, 1000.0000000000000000 1500.0000000000000000, 1500.0000000000000000 1500.0000000000000000, 1500.0000000000000000 1000.0000000000000000, 1000.0000000000000000 1000.0000000000000000))',
		'MULTIPOINT (2000.0000000000000000 2000.0000000000000000, 2000.0000000000000000 1900.0000000000000000)',
		'MULTILINESTRING ((600.0000000000000000 1500.0000000000000000, 1600.0000000000000000 2500.0000000000000000), (700.0000000000000000 1500.0000000000000000, 1700.0000000000000000 2500.0000000000000000))'
	);

	for my $orig (@wkt_list)
	{
		my $shp = mapscript::shapeObj::fromWKT( $orig );
		ok( $shp, 'create shapeObj');
		my $new_wkt = $shp->toWKT();
		ok( $orig eq $new_wkt, 'from WKT <> toWKT' );
	}
};

=pod
my $map = new mapscript::mapObj( "../mspython/test_mapio.map" );
mapscript::msIO_installStdoutToBuffer();
my $owreq = new mapscript::OWSRequest();
$owreq->loadParamsFromURL('service=WMS&version=1.1.1&request=GetMap&layers=grey&srs=EPSG:4326&bbox=-180,-90,180,90&format=image/png&width=80&height=40')

my $err = $map->OWSDispatch( $owreq );
ok($err == 0, 'OWSDispatch');

#	warn(mapscript::msGetErrorString("\n"));

my $content_type = mapscript::msIO_stripStdoutBufferContentType();
$x = mapscript::msIO_getStdoutBufferBytes();
mapscript::msIO_resetHandlers();

$res->status(200);
$res->content_type($content_type);
$res->body($$x);
=cut


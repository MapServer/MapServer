#!/usr/bin/perl
use strict;
use warnings;
use Test::More tests => 10;
use mapscript;

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

# we need to search up directories for the test folder
my $mapfile = '../../../msautotest/mspython/test_mapio.map';
if (! -e $mapfile)
{
	$mapfile = '../../../../msautotest/mspython/test_mapio.map';
}
ok(-e $mapfile, 'mapfile exists');

my $map = new mapscript::mapObj($mapfile);
ok($map, 'create mapObj');

mapscript::msIO_installStdoutToBuffer();
my $owreq = new mapscript::OWSRequest();
ok($owreq, 'create OWSRequest');
$owreq->loadParamsFromURL('service=WMS&version=1.1.1&request=GetMap&layers=grey&srs=EPSG:4326&bbox=-180,-90,180,90&format=image/png&width=80&height=40&STYLES=');
ok($owreq->getName(0) eq 'service');
ok($owreq->getValue(0) eq 'WMS');
ok($owreq->getValueByName('request') eq 'GetMap');

my $err = $map->OWSDispatch( $owreq );
ok($err == 0, 'OWSDispatch');

#	warn(mapscript::msGetErrorString("\n"));

my $h  = mapscript::msIO_getAndStripStdoutBufferMimeHeaders();
#my $k = $h->nextKey();
#while ($k)
#{
#	diag("$k == " . $h->get($k));
#	$k = $h->nextKey($k);
#}

ok($h->get('Cache-Control') eq 'max-age=86400');
ok($h->get('Content-Type') eq 'image/png');


my $x = mapscript::msIO_getStdoutBufferBytes();
#open(F, '>', 'x.png');
#print F $$x;
#close(F);

ok(substr($$x, 1, 3) eq 'PNG');



# File : Makefile.PL
use ExtUtils::MakeMaker;

open(STREAM, '../../perlvars') or die('Unable to open perlvars, have you built the MapServer yet?'); 

$home = <STREAM>;
chomp $home;

$define = <STREAM>;
chomp $define;

$inc = <STREAM>;
chomp $inc;
$inc =~ s/\-I\./\-I$home/g;

$libs = <STREAM>;
chomp $libs;
$libs =~ s/\-L\./\-L$home/g;

print $inc."\n";
print $libs."\n";

WriteMakefile(
	      'NAME' => 'mapscript',
	      'DEFINE' => $define,
	      'INC' => $inc,
	      'LIBS' => [$libs],
	      'OBJECT' => 'mapscript_wrap.o'
	      );

# File : Makefile.PL
use ExtUtils::MakeMaker;

open(STREAM, '../../mapscriptvars') or die('Unable to open mapscriptvars, have you built the MapServer yet?'); 

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

$static_libs = <STREAM>;
chomp $static_libs;
$static_libs =~ s/\-L\./\-L$home/g;

print $inc."\n";
print $libs."\n";
print $static_libs."\n";

WriteMakefile(
	      'NAME' => 'mapscript',
	      'DEFINE' => $define,
	      'INC' => $inc,
	      'LIBS' => [$libs,$static_libs],
	      'OBJECT' => 'mapscript_wrap.o'
	      );

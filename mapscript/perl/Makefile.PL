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

# Get MapServer version from mapscriptvars
$ms_version_line = <STREAM>;
if ($ms_version_line) {
	$ms_version_line =~ /MS_VERSION "(.+)"/;
	$ms_version = $1;
}
# Default is 4.3
else { $ms_version = '4.3'; }

print $inc."\n";
print $libs."\n";
print $static_libs."\n";
print $ms_version."\n";

WriteMakefile(
	      'NAME' 		=> 'mapscript',
	      'DEFINE' 		=> $define,
	      'INC' 		=> $inc,
	      'LIBS' 		=> [$libs,$static_libs],
	      'OBJECT' 		=> 'mapscript_wrap.o',
		  'VERSION' 	=> $ms_version
	      );


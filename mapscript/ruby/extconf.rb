#!/usr/bin/env ruby 
require 'mkmf'

mapscriptvars = File.open("../../mapscriptvars")
make_home = mapscriptvars.gets.chomp
make_define = mapscriptvars.gets.chomp
make_inc = mapscriptvars.gets.chomp
make_libs = mapscriptvars.gets.chomp
make_static_libs = mapscriptvars.gets.chomp
mapscriptvars.close

# $CFLAGS works only with 1.8 ??? -> the -Wall argument is not needed !!!
$CFLAGS = ""
$CPPFLAGS = make_inc + " -idirafter $(rubylibdir)/$(arch) " + make_define
$LDFLAGS += " -fPIC"
#$LOCAL_LIBS += " -L../../.libs/ " + " -lmapserver " + make_static_libs
$LOCAL_LIBS += " -L../../.libs/ " + " -lmapserver "

# if the source file 'mapscript_wrap.c' is missing nothing works
# this is a workaround !!
if !FileTest.exist?("mapscript_wrap.c")
	$objs = []
	$objs.push("mapscript_wrap.o")
end

create_makefile("mapscript")

make_file = File.open("Makefile", "a")
make_file << "\nmapscript_wrap.c: ../mapscript.i\n\tswig -ruby -o mapscript_wrap.c ../mapscript.i"
make_file.close


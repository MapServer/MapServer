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
$CPPFLAGS = " -idirafter $(rubylibdir)/$(arch) " + make_define
$LDFLAGS += " -fPIC"
$LOCAL_LIBS += " -L../.. " + make_libs + " " + make_static_libs

# variable overwritten this directories have to been included with '-idirafter' (not '-I')
$topdir = "."
$hdrdir = "."

# if the source file 'mapscript_wrap.c' is missing nothing works
# this is a workaround !!
if !FileTest.exist?("mapscript_wrap.c")
	$objs = []
	$objs.push("mapscript_wrap.o")
end

#cmake = find_executable('swig')
#cmake &&= find_executable('make')
#if cmake
  create_makefile("mapscript")
	make_file = File.open("Makefile", "a")
	make_file << "mapscript_wrap.c: mapscript.i\n\tswig -ruby mapscript.i"
	make_file.close
	exit 0
#end

#print "\nnot possible to execute the Makefile: the program swig is missing!\n"
#exit 1


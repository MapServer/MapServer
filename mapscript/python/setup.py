# $Id$
#
# setup.py file for MapScript
# version 1.0

# BUILD
#   python setup.py build
#
# INSTALL (usually as root)
#   python setup.py install

from distutils.core import setup, Extension
from distutils import sysconfig

import os.path
import string

# Function needed to make unique lists.
def unique(list):
  dict = {}
  for item in list:
    dict[item] = ''
  return dict.keys()

# Should be created by the mapserver build process.
mapscriptvars = "../../mapscriptvars"

# Open and read lines from mapscriptvars.
fp = open(mapscriptvars, "r")

ms_install_dir = fp.readline()
ms_macros = fp.readline()
ms_includes = fp.readline()
ms_libraries_pre = fp.readline()
ms_extra_libraries = fp.readline()

# Distutils wants a list of library directories and
# a seperate list of libraries.  Create both lists from
# lib_opts list.
lib_opts = string.split(ms_libraries_pre)
lib_dirs = [x[2:] for x in lib_opts if x[:2] == "-L"]
lib_dirs = unique(lib_dirs)
lib_dirs = lib_dirs + string.split(ms_install_dir)

libs = []
for x in lib_opts:
    if x[:2] == '-l':
        libs.append( x[2:] )
    if x[-4:] == '.lib' or x[-4:] == '.LIB':
	dir, lib = os.path.split(x)
	libs.append( lib[:-4] )
	if len(dir) > 0 :
	    lib_dirs.append( dir )

libs = unique(libs)
lib_dirs = unique(lib_dirs)

# Create list of macros used to create mapserver.
ms_macros = string.split(ms_macros)
macros = [(x[2:], None) for x in ms_macros]

# Create list of include directories to create mapserver.
include_dirs = [sysconfig.get_python_inc()]
ms_includes = string.split(ms_includes)
for item in ms_includes:
    if item[:2] == '-I' or item[:2] == '/I':
	if item[2:] not in include_dirs:
	    include_dirs.append( item[2:] )

# Here is the distutils setup function that does all the magic.
# Had to specify 'extra_link_args = ["-static", "-lgd"]' because
# mapscript requires the gd library, which on my system is static.
setup(name = "mapscript",
      version = "4.3",
      description = "Python interface to MapServer objects.",
      author = "Mapserver project - SWIGged MapScript library.",
      url = "http://mapserver.gis.umn.edu/",
      ext_modules = [Extension("_mapscript",
                               ["mapscript_wrap.c", "pygdioctx/pygdioctx.c"],
                               include_dirs = include_dirs,
                               library_dirs = lib_dirs,
                               libraries = libs,
                               define_macros =  macros,
                               # Uncomment line below if using static gd
                               extra_link_args = ["/NODEFAULTLIB:libc"],
                              )
                    ],
      py_modules = ["mapscript"]
     )


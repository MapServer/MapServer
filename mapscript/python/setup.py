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
lib_dirs.sort()

libs = [x[2:] for x in lib_opts if x[:2] == "-l"]
libs = unique(libs)
libs.sort() 

# Create list of macros used to create mapserver.
ms_macros = string.split(ms_macros)
macros = [(x[2:], None) for x in ms_macros]

# Here is the distutils setup function that does all the magic.
# Had to specify 'extra_link_args = ["-static", "-lgd"]' because
# mapscript requires the gd library, which on my system is static.
setup(name = "mapscript",
      version = "3.7",
      description = "Enables Python to manipulate shapefiles.",
      author = "Mapserver project - SWIGged MapScript library.",
      url = "http://mapserver.gis.umn.edu",
      ext_modules = [Extension("_mapscript", ["mapscript_wrap.c"],
                               include_dirs = [sysconfig.get_python_inc()],
                               library_dirs = lib_dirs,
                               libraries = libs,
                               define_macros =  macros,
                               extra_link_args = ["-static", "-lgd"],
                              )
                    ],
      py_modules = ["mapscript"]
     )


setup(name = "dbfreader",
      py_modules = ["dbfreader"]
     )

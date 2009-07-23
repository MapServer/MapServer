# $Id$
#
# setup.py file for MapScript
#
# BUILD
#   python setup.py build
#
# INSTALL (usually as root)
#   python setup.py install


from distutils.core import setup, Extension
from distutils import sysconfig

import sys
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
try:
	fp = open(mapscriptvars, "r")
except IOError, e:
	raise IOError, '%s. %s' % (e, "Has MapServer been made?")

ms_install_dir = fp.readline()
ms_macros = fp.readline()
ms_includes = fp.readline()
ms_libraries_pre = fp.readline()
ms_extra_libraries = fp.readline()

# Get mapserver version from mapscriptvars, which contains a line like
# 
# MS_VERSION "4.x.y"
ms_version = '4.4' # the default
ms_version_line = fp.readline()
if ms_version_line:
	ms_version = ms_version_line.split()[2]
	ms_version = ms_version.replace('"', '')
	
# Distutils wants a list of library directories and
# a seperate list of libraries.  Create both lists from
# lib_opts list.
lib_opts = string.split(ms_libraries_pre)
lib_dirs = [x[2:] for x in lib_opts if x[:2] == "-L"]
lib_dirs = unique(lib_dirs)
lib_dirs = lib_dirs + string.split(ms_install_dir)

libs = []
extras = []
ex_next = False

for x in lib_opts:
    if ex_next:
        extras.append(x)
        ex_next = False
    elif x[:2] == '-l':
        libs.append( x[2:] )
    elif x[-4:] == '.lib' or x[-4:] == '.LIB':
	    dir, lib = os.path.split(x)
	    libs.append( lib[:-4] )
	    if len(dir) > 0:
	        lib_dirs.append( dir )
    elif x[-2:] == '.a':
        extras.append(x)
    elif x[:10] == '-framework':
        extras.append(x)
        ex_next = True
    elif x[:2] == '-F':
        extras.append(x)
        
libs = unique(libs)

# if we're msvc, just link against the stub lib
# and be done with it
if sys.platform == 'win32':
    libs = ['mapserver_i','gd']

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

# Uncomment lines below if using static gd
#extras.append("-static")
#extras.append("-lgd")

setup(name = "mapscript",
      version = ms_version,
      description = "Python interface to MapServer",
      author = "MapServer Project",
      url = "http://mapserver.gis.umn.edu/",
      ext_modules = [Extension("_mapscript",
                               ["mapscript_wrap.c", "pygdioctx/pygdioctx.c"],
                               include_dirs = include_dirs,
                               library_dirs = lib_dirs,
                               libraries = libs,
                               define_macros =  macros,
                               extra_link_args = extras,
                              )
                    ],
      py_modules = ["mapscript"]
     )


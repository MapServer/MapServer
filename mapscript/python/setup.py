#! /usr/bin/env python
# Distutils setup for UMN Mapserver mapscript v.3.5
# Norman Vine 11/10/2001

# On cygwin this will require my Distutils mods available @
# http://www.vso.cape.com/~nhv/files/python/mydistutils.txt
# Should work as is on 'Unix' like systems
# needs testing on Windows compilers other then Cygwin

# To Install
# build mapserver
# Copy this file to $MAPSERVER_SRC / mapscript / python
# invoke as ./setup.py install

# should work on any python version with Distutils

__revision__ = "$Id$"

from distutils.core import setup, Extension
from distutils.spawn import spawn
from distutils.dir_util import mkpath
from distutils.file_util import copy_file
from distutils.sysconfig import parse_makefile

import os
from os import path

noisy=1
swig_cmd = ["swig",
            "-python",
            "-shadow",
            "-opt",
            "-DPYTHON",
            "-module",
            "mapscript",
            "-o",
            "./mapscript_wrap.c",
            "../mapscript.i" ]
            
spawn(swig_cmd, verbose=noisy)

# make package directory and package __init__ script
mkpath("mapscript")
init_file=open(path.join("mapscript","__init__.py"),"w")
init_file.write("from mapscript import *\n")
init_file.close()

copy_file("mapscript.py", path.join("mapscript","mapscript.py"), verbose=noisy)

# change to reflect the gd version you are using
gd_dir="gd-1.8.4"
ms_dir=path.join("..","..")
local_dir="/usr/local"

setup (# Distribution meta-data
       name = "pymapscript",
       version = "3.5",
       description = "pre release",
       author = "Steve Lime",
       author_email = "steve.lime@dnr.state.mn.us",
       url = "http://mapserver.gis.umn.edu/",

       # Description of the modules and packages in the distribution
       packages = ['mapscript'],
       ext_modules = 
           [Extension('mapscriptc', ['mapscript_wrap.c'],
                      define_macros=[('TIFF_STATIC',),('JPEG_STATIC',),('ZLIB_STATIC',),
                          ('IGNORE_MISSING_DATA',),('USE_EPPL',),('USE_PROJ',),('USE_PROJ_API_H',),
                          ('USE_WMS',),('USE_TIFF',),('USE_JPEG',),('USE_GD_PNG',),('USE_GD_JPEG',),
                          ('USE_GD_WBMP',),('USE_GDAL',),('USE_POSTGIS',)],
                      include_dirs=[ms_dir,
#                          path.join(ms_dir,gd_dir),
#                          path.join(ms_dir,"gdft"),
#                          path.join(local_dir,'include/freetype'),
                          path.join(local_dir,'include')
                                    ],
                      library_dirs=[ms_dir,
#                          path.join(ms_dir,gd_dir),
#                          path.join(ms_dir,"gdft"),
                          '/lib',
                          path.join(local_dir,'lib')],
                      extra_objects=[path.join(ms_dir,'libmap.a')],
#                      libraries=['gd','gdft','freetype','ttf','proj','pq','png','z'],),
                      libraries=['gdal.1.1', 'gd','freetype','ttf','png','proj','z',
                                 'wwwxml','xmltok','xmlparse','wwwinit','wwwapp','wwwhtml','wwwtelnet','wwwnews','wwwhttp','wwwmime','wwwgopher','wwwftp','wwwfile','wwwdir','wwwcache','wwwstream','wwwmux','wwwtrans','wwwcore','wwwutils','md5','ming'
                                 ],),                          
           ]
      )

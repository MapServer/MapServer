# $Id$
#
# setup.py file for MapScript
#
# BUILD
#   python setup.py build
#
# INSTALL (usually as root)
#   python setup.py install
# 
# DEVELOP (build and run in place)
#   python setup.py develop

import sys, os

HAVE_SETUPTOOLS = False
try:
    from setuptools import setup
    from setuptools import Extension
    HAVE_SETUPTOOLS = True
except ImportError:
    from distutils.core import setup, Extension


from distutils import sysconfig
from distutils.command.build_ext import build_ext
from distutils.ccompiler import get_default_compiler
from distutils.sysconfig import get_python_inc

try:
    import subprocess
except ImportError:
    import popen2

def update_dirs(list1, list2):
    for v in list2:
        if v not in list1 and os.path.isdir(v):
            list1.append(v)

# 
# # Function needed to make unique lists.
def unique(list):
    ret_list = []
    dict = {}
    for item in list:
        if not item in dict:
            dict[item] = ''
            ret_list.append( item )
    return ret_list


# ---------------------------------------------------------------------------
# Default build options
# (may be overriden with setup.cfg or command line switches).
# ---------------------------------------------------------------------------

include_dirs = ['../..']
library_dirs = ['../../.libs']
libraries = ['mapserver']

extra_link_args = []
extra_compile_args = []
# might need to tweak for Python 2.4 on OSX to be these
#extra_compile_args = ['-g', '-arch', 'i386', '-isysroot','/']


save_init_posix = sysconfig._init_posix
def ms_init_posix():
    save_init_posix()
    if 'LDCXXSHARED' in sysconfig._config_vars:
        sysconfig._config_vars['LDSHARED'] = sysconfig._config_vars['LDCXXSHARED']
sysconfig._init_posix = ms_init_posix



def read_mapscriptvars():

    # Should be created by the mapserver build process.
    mapscriptvars = "../../mapscriptvars"

    try:
        fp = open(mapscriptvars, "r")
    except IOError as e:
        raise IOError('%s. Has MapServer been made?' % e)
    
    output = {}
    install_dir = fp.readline().strip()
    defines = fp.readline().strip()
    includes = fp.readline().strip()
    libraries = fp.readline().strip()
    extra_libs = fp.readline().strip()
    version = fp.readline().strip()
    if version:
        version = version.split()[2]
        version = version.replace('#','')
        version = version.replace('"','')
    
    output['version'] = version
    output['libs'] = libraries
    output['extra_libs'] = extra_libs
    output['defines'] = defines
    output['includes'] = includes
    fp.close()
   # print output['libs']
   # sys.exit(1)
    return output

def get_config(option, config='mapserver-config'):
    
    v = read_mapscriptvars()
    if sys.platform == 'win32':
        v = read_mapscriptvars()
        return v[option]
    command = config + " --%s" % option
    try:
        subprocess
        command, args = command.split()[0], command.split()[1]
        p = subprocess.Popen([command, args], stdin=subprocess.PIPE, stdout=subprocess.PIPE, close_fds=True)
        (child_stdout, child_stdin) = (p.stdout, p.stdin)
        r = child_stdout.read().strip()
    except NameError:
        p = popen2.popen3(command)
        r = p[0].readline().strip()
        if not r:
            raise Warning(p[2].readline())
    return r
    

class ms_ext(build_ext):

    MAPSERVER_CONFIG = 'mapserver-config'
    user_options = build_ext.user_options[:]
    user_options.extend([
        ('mapserver-config=', None,
        "The name of the mapserver-config binary and/or a full path to it"),
    ])

    def initialize_options(self):
        print("LD_RUN_PATH set")
        os.environ["LD_RUN_PATH"] = os.getcwd()+"/../../.libs"
        build_ext.initialize_options(self)
        self.gdaldir = None
        self.mapserver_config = self.MAPSERVER_CONFIG

    def get_compiler(self):
        return self.compiler or get_default_compiler()
    
    def get_mapserver_config(self, option):
        return get_config(option, config =self.mapserver_config)
    
    def finalize_options(self):
        if isinstance(self.include_dirs, str):
            self.include_dirs = [path.strip() for path in self.include_dirs.strip().split(":")]
        if self.include_dirs is None:
            self.include_dirs = include_dirs

        update_dirs(self.include_dirs, include_dirs)
        
        includes =  self.get_mapserver_config('includes')
        includes = includes.split()
        for item in includes:
            if item[:2] == '-I' or item[:2] == '/I':
                if item[2:] not in include_dirs:
                    self.include_dirs.append( item[2:] )

        if isinstance(self.library_dirs, str):
            self.library_dirs = [path.strip() for path in self.library_dirs.strip().split(":")]
        if self.library_dirs is None:
            self.library_dirs = library_dirs

        update_dirs(self.library_dirs, library_dirs)

        libs =  self.get_mapserver_config('libs')
        self.library_dirs = self.library_dirs + [x[2:] for x in libs.split() if x[:2] == "-L"]

        # silly stuff to deal with setuptools doing things 
        # like -D-DMYDEFINE
        defs = self.get_mapserver_config('defines')
        self.define = [x[2:] for x in defs.split() if x[:2] == "-D"]
        self.define = ','.join(self.define)

        ex_next = False
        libs = libs.split()
        for x in libs:
            if ex_next:
                extra_link_args.append(x)
                ex_next = False
            elif x[:2] == '-l':
                libraries.append( x[2:] )
            elif x[-4:] == '.lib' or x[-4:] == '.LIB':
                dir, lib = os.path.split(x)
                libraries.append( lib[:-4] )
                if len(dir) > 0:
                    self.library_dirs.append( dir )
            elif x[-2:] == '.a':
                extra_link_args.append(x)
            elif x[:10] == '-framework':
                extra_link_args.append(x)
                ex_next = True
            elif x[:2] == '-F':
                extra_link_args.append(x)
                
        # don't forget to add mapserver lib
        self.libraries = unique(libraries) + ['mapserver',]

        if self.get_compiler() == 'msvc':
            for lib in self.libraries:
                if lib == 'mapserver':
                    self.libraries.remove(lib)
            self.libraries.append('mapserver_i')
            self.libraries.append('gd')
            self.libraries = unique(self.libraries)
        build_ext.finalize_options(self)
        
        self.dir = os.path.abspath('../..')
        self.library_dirs.append(self.dir)
        self.include_dirs.append(self.dir)


    
mapserver_module = Extension('_mapscript',
                        sources=["mapscript_wrap.c"],
#                        define_macros = define_macros,
                        extra_compile_args = extra_compile_args,
                        extra_link_args = extra_link_args)


mapserver_version = get_config('version', config='../../mapserver-config')

author = "Steve Lime"
author_email = "steve.lime@dnr.state.mn.us"
maintainer = "Howard Butler"
maintainer_email = "hobu.inc@gmail.com"
description = "MapServer Python MapScript bindings"
license = "MIT"
url="http://www.mapserver.org"
name = "MapScript"
ext_modules = [mapserver_module,]
py_modules = ['mapscript',]

with open('README', 'r') as fh:
    readme = fh.read()

if not os.path.exists('mapscript_wrap.c') :
    swig_cmd = """swig -python -shadow -modern -templatereduce -fastdispatch -fvirtual -fastproxy -modernargs -castmode -dirvtable -fastinit -fastquery -noproxydel -nobuildnone %s -o mapscript_wrap.c ../mapscript.i"""
    os.system(swig_cmd % get_config('defines', config='../../mapserver-config'))

classifiers = [
        'Development Status :: 4 - Beta',
        'Intended Audience :: Developers',
        'Intended Audience :: Science/Research',
        'License :: OSI Approved :: MIT License',
        'Operating System :: OS Independent',
        'Programming Language :: Python',
        'Programming Language :: C',
        'Programming Language :: C++',
        'Topic :: Scientific/Engineering :: GIS',
        'Topic :: Scientific/Engineering :: Information Analysis',
        
]

if HAVE_SETUPTOOLS:
    setup( name = name,
           version = mapserver_version,
           author = author,
           author_email = author_email,
           maintainer = maintainer,
           maintainer_email = maintainer_email,
           long_description = readme,
           description = description,
           license = license,
           classifiers = classifiers,
           py_modules = py_modules,
           url=url,
           zip_safe = False,
           cmdclass={'build_ext':ms_ext},
           ext_modules = ext_modules )
else:
    setup( name = name,
           version = mapserver_version,
           author = author,
           author_email = author_email,
           maintainer = maintainer,
           maintainer_email = maintainer_email,
           long_description = readme,
           description = description,
           license = license,
           classifiers = classifiers,
           py_modules = py_modules,
           url=url,
           cmdclass={'build_ext':ms_ext},
           ext_modules = ext_modules )    

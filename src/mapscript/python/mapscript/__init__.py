import sys
import platform
import os
import inspect

# As of Python 3.8 PATH can no longer be used to resolve the MapServer
# DLLs on Windows. Instead users will be required to set a new MAPSERVER_DLL_PATH
# environment variable.
# See https://docs.python.org/3/whatsnew/3.8.html#changes-in-the-python-api


def add_dll_path(pth):
    """
    Allow Python to access the directory containing the MapServer DLL (for Python on Windows only)
    """
    if (3, 8) <= sys.version_info:
        os.add_dll_directory(pth)
    else:
        # add the directory to the Windows path for earlier Python version
        os.environ['PATH'] = pth + ';' + os.environ['PATH']


if platform.system() == 'Windows':
    mapserver_dll_path = os.getenv('MAPSERVER_DLL_PATH', '')
    dll_paths = list(filter(os.path.exists, mapserver_dll_path.split(';')))
    # add paths in the order listed in the string
    dll_paths.reverse()
    for pth in dll_paths:
        add_dll_path(pth)


from .mapscript import *


# change all the class module names from mapscript.mapscript to mapscript

for key, value in globals().copy().items():
    if inspect.isclass(value) and value.__module__.startswith('mapscript.'):
        value.__module__= 'mapscript'

# remove the submodule name

del mapscript

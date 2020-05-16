import inspect
from .mapscript import *

# change all the class module names from mapscript.mapscript to mapscript

for key, value in globals().copy().items():
    if inspect.isclass(value) and value.__module__.startswith('mapscript.'):
        value.__module__= 'mapscript'

# remove the submodule name

del mapscript

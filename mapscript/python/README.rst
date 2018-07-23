Python MapScript for MapServer 7.2 README
=========================================

:Author: MapServer Team
:Last Updated: 2018-07-23

Introduction
------------

The Python mapscript module provides users an interface to `MapServer <http://mapserver.org>`_
classes on any platform, and has been tested on Python versions 2.7 and 3.5+. 

The Python mapscript module is created using `SWIG <http://www.swig.org.>`_ the
the Simplified Wrapper and Interface Generator. This is used to create MapServer bindings in
many different programming languages. 

+ Language agnostic documentation is available at http://mapserver.org/mapscript/introduction.html
+ Python specific documentation is available at http://mapserver.org/mapscript/python.html

For working with Mapfiles in Python the `mappyfile <https://mappyfile.readthedocs.io/en/latest/>`_ project is 
also available, this allows creating, parsing, formatting, and validating Mapfiles. 

Wheels and PyPI
---------------

Python `wheels <https://wheel.readthedocs.io/en/stable/>`_ have been created for Windows and uploaded to 
`PyPI <https://pypi.org/>`_ - the Python Package Index. This allows easy installation using `pip <https://pypi.org/project/pip/>`_. 
Other advantages of ready-made wheels on PyPI are:

+ mapscript can be added as a dependency to `Requirements Files <https://pip.pypa.io/en/stable/user_guide/#id1>`_
+ mapscript can be easily added to a Python `Virtual Environment <https://docs.python-guide.org/dev/virtualenvs/>`_
+ Python2 or Python3 versions of mapscript can be installed and work with a single installation of MapServer

Currently the following wheels are built:

+ Python 2.7 x64 for MapServer 7.2
+ Python 3.6 x64 for MapServer 7.2

Linux wheels are also planned, using the `manylinux <https://github.com/pypa/manylinux>`_ project. 

No source distributions will be provided on PyPI - to build from source requires the full MapServer source code,
in which case it is easiest to take a copy of the full MapServer project and run the CMake process detailed below. 

..
    py3 SWIG flag adds type annotations

MapServer Versions
------------------

To use mapscript you will need to add the MapServer binaries to your system path. 
On Windows you can use the following, replacing ``C:\MapServer\bin`` with the location of your MapServer binaries. 

.. code-block::

    SET PATH=C:\MapServer\bin;%PATH%

Window binary packages can be downloaded from `GIS Internals <https://www.gisinternals.com/stable.php>`. 
When using these packages the MapServer path will be similar to `C:\release-1911-x64-gdal-mapserver\bin`. 

The mapscript wheels have been compiled using Visual Studio 2017 version 15.3 (``MSVC++ 14.11 _MSC_VER == 1911``). 

Installation
------------

Prior to installing it is first recommended to update pip to the latest version with the following command:

.. code-block::

    python -m pip install --upgrade pip

Next if there are binary wheels available for your system, mapscript can be installed using:

.. code-block::

    pip install mapscript

If you already have mapscript installed and wish to upgrade it to a newer version you can use:

.. code-block::

    pip install mapscript --upgrade

Add your MapSever binaries folder to your system path (see `MapServer Versions`_):

.. code-block::

    SET PATH=C:\MapServer\bin;%PATH%

Now you should be able to import mapscript:

.. code-block:: python

    python -c "import mapscript;print(mapscript.msGetVersion())"
    MapServer version 7.2.0-beta2 OUTPUT=PNG OUTPUT=JPEG OUTPUT=KML SUPPORTS=PROJ SUPPORTS=AGG SUPPORTS=FREETYPE SUPPORTS=CAIRO SUPPORTS=SVG_SYMBOLS SUPPORTS=SVGCAIRO SUPPORTS=ICONV SUPPORTS=FRIBIDI SUPPORTS=WMS_SERVER SUPPORTS=WMS_CLIENT SUPPORTS=WFS_SERVER SUPPORTS=WFS_CLIENT SUPPORTS=WCS_SERVER SUPPORTS=SOS_SERVER SUPPORTS=FASTCGI SUPPORTS=THREADS SUPPORTS=GEOS SUPPORTS=PBF INPUT=JPEG INPUT=POSTGIS INPUT=OGR INPUT=GDAL INPUT=SHAPEFILE

If you failed to add the MapServer binaries to your system path you may see one of the following errors:

.. code-block:: python

    ImportError: No module named _mapscript # Python 2.x
    ModuleNotFoundError: No module named '_mapscript' # Python 3.x

If your version of mapscript does not match your version of MapServer you may instead one of the following messages:

.. code-block:: python

    ImportError: DLL load failed: The specified module could not be found.
    ImportError: DLL load failed: The specified procedure could not be found.

Quickstart
----------

Prior to running any scripts using mapscript, you will need to add the MapServer binaries to your system path, see the
*Installation* section above. 

To open an existing Mapfile:

.. code-block:: python

    >>> import mapscript
    >>> test_map = mapscript.mapObj(r"C:\Maps\mymap.map")
    >>> e = test_map.extent

Create a layer from a string:

.. code-block:: python

    >>> import mapscript
    >>> lo = mapscript.fromstring("""LAYER NAME "test" TYPE POINT END""")
    >>> lo
    <mapscript.layerObj; proxy of C layerObj instance at ...>
    >>> lo.name
    'test'
    >>> lo.type == mapscript.MS_LAYER_POINT
    True

Building the Mapscript Module
-----------------------------

The mapscript module is built as part of the MapServer CMake build process, this 
is configured using the ``mapserver/mapscript/CMakeLists.txt`` file. 

Prior to the switch to using CMake to build MapServer mapscript was built using
distutils and ``setup.py``. Now the ``setup.py.in`` file is used as a template that
is filled with the MapServer version number and used to created wheel files for distribution. 

The build process works as follows. 

+ CMake runs SWIG. This uses the SWIG interface files to create a ``mapscriptPYTHON_wrap.c`` file, 
  and a ``mapscript.py`` file containing the Python wrapper to the mapscript binary module. 
+ CMake then uses the appropriate compiler on the system to compile the ``mapscriptPYTHON_wrap.c`` file into a Python binary module -
  ``_mapscript.pyd`` file on Windows, and a ``_mapscript.so`` file on Windows. 

``CMakeLists.txt`` is configured so that all files required to make a Python wheel are copied into the output build folder. The wheel can then be built
with the following commands. 

.. code-block:: bat

    python -m pip install --upgrade pip
    pip install wheel
    cd C:\Projects\MapServer\build\mapscript\python
    python setup.py bdist_wheel

SWIG can be run manually, without using CMake. This may allow further optimizations and control on the output. 

.. code-block:: bat

    cd C:\Projects\mapserver\build
    SET PATH=C:\MapServerBuild\swigwin-3.0.12;%PATH%
    swig -python -shadow -o mapscript_wrap.c ../mapscript.i

SWIG has several command line options to control the output:

.. code-block:: bat
    
    swig -python -shadow -modern -templatereduce -fastdispatch -fvirtual -fastproxy 
    -modernargs -castmode -dirvtable -fastinit -fastquery -noproxydel -nobuildnone 
    -o mapscript_wrap.c ../mapscript.i

Testing
-------

Once the mapscript module has been built there is a test suite to check the output. It is recommended
`pytest <https://docs.pytest.org/en/latest/>`_ is used to run the tests. This can be installed using:

.. code-block:: bat

    pip install pytest

Change the directory to the mapscript output build folder and run the command below. Some tests are currently excluded, these will
be fixed for upcoming releases. It is also planned to include the test suite in the Python wheels to allow easy testing of a 
mapscript installation. 

.. code-block:: bat

    python -m pytest --ignore=tests/cases/fonttest.py --ignore=tests/cases/hashtest.py --ignore=tests/cases/pgtest.py --ignore=tests/cases/threadtest.py tests/cases

Credits
-------

+ Steve Lime (developer)
+ Sean Gillies (developer)
+ Frank Warmerdam (developer)
+ Howard Butler (developer)
+ Norman Vine (cygwin and distutils guru)
+ Tim Cera (install)
+ Michael Schultz (documentation)
+ Thomas Bonfort (developer)
+ Even Rouault (developer)
+ Seth Girvin (Python3 migration, documentation and builds)
+ Claude Paroz (Python3 migration)
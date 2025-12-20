Python MapScript for MapServer README
=====================================

:Author: MapServer Team
:Last Updated: 2025-12-20

Introduction
------------

The Python mapscript module provides users an interface to `MapServer <http://mapserver.org>`_
classes on any platform, and has been tested on Python 3.8+. 

The Python mapscript module is created using `SWIG <http://www.swig.org.>`_ the
the Simplified Wrapper and Interface Generator. This is used to create MapServer bindings in
many different programming languages. 

+ Language agnostic documentation is available at http://mapserver.org/mapscript/introduction.html
+ Python specific documentation is available at http://mapserver.org/mapscript/python.html

When working with Mapfiles in Python the `mappyfile <https://mappyfile.readthedocs.io/en/latest/>`_ project can
also be used, this allows creating, parsing, formatting, and validating Mapfiles without any dependencies on MapServer. 

Wheels and PyPI
---------------

Python `wheels <https://wheel.readthedocs.io/en/stable/>`_ for Windows are automatically and uploaded to 
`PyPI <https://pypi.org/>`_ - the Python Package Index on each MapServer release. 
Note - MapServer binaries still need to be installed on the system, and are not included in the wheel itself, 
see the *Installation* section below.

Advantages of ready-made wheels on PyPI include:

+ easy installation using `pip <https://pypi.org/project/pip/>`_
+ mapscript can be added as a dependency to `Requirements Files <https://pip.pypa.io/en/stable/user_guide/#id1>`_
+ mapscript can be easily added to a Python `Virtual Environment <https://docs.python-guide.org/dev/virtualenvs/>`_
+ Python2 or Python3 versions of mapscript can be installed and work with a single installation of MapServer

Wheels are built based on the `Appveyor build environments <https://github.com/MapServer/MapServer/blob/main/appveyor.yml>`_. 
These are as follows at the time of writing:

+ Python 3.10 x64
+ Python 3.11 x64
+ Python 3.12 x64
+ Python 3.13 x64
+ Python 3.14 x64

The mapscript wheels have been compiled using Visual Studio 2022 version 17 (``MSVC++ 17.9 _MSC_VER == 1939``). 
Linux Wheels may also be available in the future using the `manylinux <https://github.com/pypa/manylinux>`_ project. 

No source distributions will be provided on PyPI - to build from source requires the full MapServer source code,
in which case it is easiest to take a copy of the full MapServer project and run the CMake process detailed below. 

The Wheels contain a full test suite and sample data that can be run to check that the installed MapServer is
running correctly. 

..
    py3 SWIG flag adds type annotations

Installation on Windows
-----------------------

To use mapscript you will need to add the MapServer binaries to your system path. 

For Python 3.8+
+++++++++++++++

As of `Python 3.8 <https://docs.python.org/3/whatsnew/3.8.html#changes-in-the-python-api>`_ ``PATH`` 
and the current working directory are no longer used when searching for the MapServer DLLs.
A new environment variable ``MAPSERVER_DLL_PATH`` has been introduced to set the location of the MapServer DLLs.
On Windows you can use the following, replacing ``C:\MapServer\bin`` with the location of your MapServer binaries.

.. code-block:: bat

    SET MAPSERVER_DLL_PATH=C:\MapServer\bin

If several folders are required (e.g. GDAL DLLs) multiple paths can be provided separated by semi-colons:

.. code-block:: bat

    SET MAPSERVER_DLL_PATH=C:\MapServer\bin;C:\GDAL\bin

In PowerShell you can set this as follows:

.. code-block:: ps1

    $env:MAPSERVER_DLL_PATH="C:\MapServer\bin"

For Earlier Python Versions
+++++++++++++++++++++++++++

For Python 3.7 and earlier (including Python 2.7) you can use either the ``MAPSERVER_DLL_PATH`` variable documented above,
or the system ``PATH`` variable as below, replacing ``C:\MapServer\bin`` with the location of your MapServer binaries. 

.. code-block:: bat

    SET PATH=C:\MapServer\bin;%PATH%

Windows Binaries
++++++++++++++++

Windows binary packages can be downloaded from `GIS Internals <https://www.gisinternals.com/stable.php>`_. 
To ensure compatibility with the wheels, please use identical release packages, e.g. ``release-1930-x64-gdal-3-11-3-mapserver-8-4-0``
for mapscript 8.4.0. 

.. NOTE::
   `MS4W <https://www.ms4w.com>`_ (MapServer for Windows) is a full installer that contains Python & Python
   MapScript already configured out-of-the-box, as well as default OGC web services and over 60 working mapfiles.

When using these packages the MapServer path will be similar to ``C:\release-1930-x64-gdal-3-11-3-mapserver-8-4-0\bin``. 

Prior to installing mapscript it is recommended to update pip to the latest version with the following command:

.. code-block:: bat

    python -m pip install --upgrade pip

If there are binary wheels available for your system, mapscript can be installed using:

.. code-block:: bat

    pip install mapscript

If you already have mapscript installed and wish to upgrade it to a newer version you can use:

.. code-block:: bat

    pip install mapscript --upgrade

Now you should be able to import mapscript:

.. code-block:: python

    python -c "import mapscript;print(mapscript.msGetVersion())"
    MapServer version 8.6 PROJ version 9.6 GDAL version 3.13 OUTPUT=PNG OUTPUT=JPEG SUPPORTS=PROJ SUPPORTS=AGG SUPPORTS=FREETYPE SUPPORTS=CAIRO SUPPORTS=SVG_SYMBOLS SUPPORTS=SVGCAIRO SUPPORTS=ICONV SUPPORTS=FRIBIDI SUPPORTS=WMS_SERVER SUPPORTS=WMS_CLIENT SUPPORTS=WFS_SERVER SUPPORTS=WFS_CLIENT SUPPORTS=WCS_SERVER SUPPORTS=OGCAPI_SERVER SUPPORTS=FASTCGI SUPPORTS=THREADS SUPPORTS=GEOS SUPPORTS=PBF INPUT=JPEG INPUT=POSTGIS INPUT=OGR INPUT=GDAL INPUT=SHAPEFILE INPUT=FLATGEOBUF

Installation on Unix
--------------------

For Unix users there are two approaches to installing mapscript. The first is to install the ``python3-mapscript`` package using a package manager. For example on
Ubuntu the following command can be used:

.. code-block:: bat

    sudo apt-get install python3-mapscript

The second approach is to build and install the Python mapscript module from source. Full details on compiling MapServer from source are detailed on the
`Compiling on Unix <https://www.mapserver.org/installation/unix.html>`_ page. To make sure Python mapscript is built alongside MapServer the following flag needs to be set:

.. code-block:: bat

    -DWITH_PYTHON=ON

To configure the path of the mapscript installation location ``-DCMAKE_INSTALL_PREFIX`` can be set, e.g. 

.. code-block:: bat

    sudo cmake .. -DCMAKE_INSTALL_PREFIX=/usr

When installing the `DESTDIR <https://cmake.org/cmake/help/latest/envvar/DESTDIR.html>`_ variable can be set (note ``DESTDIR`` is not used on Windows)
to install mapscript to a non-default location. E.g.

.. code-block:: bat

    make install DESTDIR=/tmp

In summary the ``install`` target runs the ``setup.py install`` command using custom paths (when set) similar to below:

    python setup.py install --root=${DESTDIR} --prefix={CMAKE_INSTALL_PREFIX}

Installation Troubleshooting
----------------------------

If the ``_mapscript.pyd`` (or ``_mapscript.so`` on Unix) is missing from the `Lib/site-packages/mapscript` 
folder (which can happen if the source installation is installed rather than a pre-compiled version) the following error will occur:

.. code-block:: python

    ImportError: cannot import name '_mapscript' from partially initialized module 'mapscript' (most likely due to a circular import)

If the mapscript library is not on your ``PYTHONPATH`` you may see one of the following errors:

.. code-block:: python

    ModuleNotFoundError: No module named '_mapscript' # Python 3.x

If the ``MapServer.dll`` cannot be found in your system paths (or ``MAPSERVER_DLL_PATH`` environment variable when using Python 3.8 
or higher on Windows) you will see the following message:

.. code-block:: python

    ImportError: DLL load failed: The specified module could not be found.

If MapServer has been built with a dependency also used by Python, and the versions don't match you may see the error below. 

.. code-block:: python

    ImportError: DLL load failed: The specified procedure could not be found.

This is a particular problem on Windows with ``sqlite3.dll`` as it is used by both Python and MapServer. Copying the ``sqlite3.dll``
from the MapServer binaries folder alongside ``_mapscript.pyd`` in `Lib/site-packages/mapscript` can resolve this.

Another common cause is if the Python environment contains multiple versions of the GEOS binary. For example ``geos_c.dll`` is included
as part of the Shapely Python library, as well as a MapServer installation.

If you are using 32 bit Python on Windows and attempt to use a 64 bit version of MapScript the following import error will occur:

.. code-block:: python

    ImportError: DLL load failed while importing _mapscript: %1 is not a valid Win32 application.

Quickstart
----------

Some basic examples of what can be done with mapscript are shown below. Note - before running any scripts using mapscript, 
you will need to add the MapServer binaries to your system path, see the *Installation* section above. 

To open an existing Mapfile:

.. code-block:: python

    >>> import mapscript
    >>> test_map = mapscript.mapObj(r"C:\Maps\mymap.map")
    >>> extent = test_map.extent

Create a layer from a string:

.. code-block:: python

    >>> import mapscript
    >>> layer = mapscript.fromstring("""LAYER NAME "test" TYPE POINT END""")
    >>> layer
    <mapscript.layerObj; proxy of C layerObj instance at ...>
    >>> layer.name
    'test'
    >>> layer.type == mapscript.MS_LAYER_POINT
    True

Building the Mapscript Module
-----------------------------

The mapscript module is built as part of the MapServer CMake build process. This is configured using the ``mapserver/mapscript/CMakeLists.txt`` file. 

Before the switch to CMake MapServer mapscript was built using distutils and ``setup.py``. Now the ``setup.py.in`` file is used as a template that
is filled with the MapServer version number and used to created wheel files for distribution, or install mapscript directly on the build machine.  

The build process works as follows. 

+ CMake runs SWIG. This uses the SWIG interface files to create a ``mapscriptPYTHON_wrap.c`` file, 
  and a ``mapscript.py`` file containing the Python wrapper to the mapscript binary module. 
+ CMake then uses the appropriate compiler on the system to compile the ``mapscriptPYTHON_wrap.c`` file into a Python binary module -
  ``_mapscript.pyd`` file on Windows, and a ``_mapscript.so`` file on Unix. 

``CMakeLists.txt`` is configured with a ``pythonmapscript-wheel`` target that copies all the required files to the output build folder where they are then packaged
into a Python wheel. The wheel can be built using the following command:

.. code-block:: bat

    cmake --build . --target pythonmapscript-wheel

The ``pythonmapscript-wheel`` target creates a virtual environment, creates the Python wheel, installs it to the virtual environment and finally runs the test
suite. This process runs commands similar to the following:
 
.. code-block:: bat

    python -m venv mapscriptvenv
    python -m pip install --upgrade pip
    pip install -r requirements-dev.txt
    python setup.py bdist_wheel
    pip install --no-index --find-links=dist mapscript
    python -m pytest --pyargs mapscript.tests

SWIG can also be run manually, without using CMake. This may allow further optimizations and control on the output. 

.. code-block:: bat

    cd C:\Projects\mapserver\build
    SET PATH=C:\MapServerBuild\swigwin-4.4.1;%PATH%
    swig -python -shadow -o mapscript_wrap.c ../mapscript.i

SWIG has several command line options to control the output, examples of which are shown below:

.. code-block:: bat
    
    swig -python -shadow -modern -templatereduce -fastdispatch -fvirtual -fastproxy 
    -modernargs -castmode -dirvtable -fastinit -fastquery -noproxydel -nobuildnone 
    -o mapscript_wrap.c ../mapscript.i

Testing
-------

The mapscript module includes a test suite and a small sample dataset to check the output and MapServer installation. It is recommended
`pytest <https://docs.pytest.org/en/latest/>`_ is used to run the tests. This can be installed using:

.. code-block:: bat

    pip install pytest

Make sure the MapServer binaries are on the system path, and that the PROJ_DATA variable has been set as this is required for many of the tests. 

.. code-block:: bat

    SET PATH=C:\release-1930-x64-gdal-3-11-3-mapserver-8-4-0\bin;%PATH%
    SET PROJ_DATA=C:\release-1930-x64-gdal-3-11-3-mapserver-8-4-0\bin\proj\SHARE

Finally run the command below to run the test suite: 

.. code-block:: bat

    pytest --pyargs mapscript.tests

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

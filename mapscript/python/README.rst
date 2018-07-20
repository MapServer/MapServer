Python MapScript for MapServer 7.2 README
=========================================

:Author: Howard Butler
:Contact: hobu.inc@gmail.com
:Author: Sean Gillies
:Contact: sgillies@frii.com
:Author: Seth Girvin
:Contact: sethg@geographika.co.uk
:Last Updated: 2018-07-20

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

Installation
------------

Python `wheels <https://wheel.readthedocs.io/en/stable/>`_ have been created for Windows and uploaded to 
`pypi <https://pypi.org/>`_ - the Python Package Index. 

Linux wheels are also planned, using the `manylinux <https://github.com/pypa/manylinux>`_ project. 

This allows easy installation using `pip <https://pypi.org/project/pip/>`_:

.. code-block::

    pip install mapscript

No source distributions will be provided on PyPI - to build from source requires the full MapServer source code,
in which case it is easiest to take a copy of the full MapServer project and run the CMake process detailed below. 

Quickstart
----------

Open an existing Mapfile:

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

``CMakeLists.txt`` is setup so that all files required to make a Python wheel are copied into the output build folder. The wheel can then be built
with the following commands. 

.. code-block:: bat

    python -m pip install --upgrade pip
    pip install wheel
    cd C:\Projects\MapServer\build\mapscript\python
    python setup.py bdist_wheel --plat-name=win-amd64

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
+ Even Rouault (developer)
+ Seth Girvin (Python3 migration, documentation and builds)
+ Claude Paroz (Python3 migration)
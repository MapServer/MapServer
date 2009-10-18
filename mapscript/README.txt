.. $Id$

MapServer Mapscript Modules
===========================

Much of MapServer's functionality is accessible from Java, Perl, PHP, Python,
Ruby, and Tcl.  Mapscript is not a language like Javascript or Postscript. It
is a loadable module that brings MapServer capabilities to your favorite high
level programming language.


PHP and SWIG Branches of Mapscript
----------------------------------
The mapscript family tree looks like this:

::

               MapServer
                  /\ 
                 /  \    
                /    \ 
     PHPMapScript     \  SWIGMapScript
         PHP4      +----+-----+----+----+----+
                   |    |     |    |    |    |
                 Perl Python Ruby Java Tcl  ...


The Perl, Python, Ruby flavors are like brothers and sisters and the PHP3
module is like a very close cousin to them.  The Java, Perl, Python, Ruby,
and Tcl modules are generated using SWIG (http://www.swig.org) while the
PHP3/PHP4 module is developed using the PHP C API without using SWIG.


PHP3/PHP4
---------

Source code, detailed installation instructions, and module API are located
under mapscript/php3.


SWIG Mapscript
--------------

Installation instructions are located in the individual languages
directories such as mapscript/perl, mapscript/python.  The shared API
is documented in the file mapscript/doc/mapscript.txt.

The main mapscript SWIG interface file is mapscript/mapscript.i.  This
file includes specific class interface files from mapscript/swiginc and
language specific code from the language directories.


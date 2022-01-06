MapServer
=========

| |Build Status| |Appveyor Build Status| |Coveralls Status|

-------
Summary
-------
   
MapServer is a system for developing web-based GIS applications. 
The basic system consists of a CGI program that can be configured to 
respond to a variety of spatial requests like making maps, scalebars, 
and point, area and feature queries. Virtually all aspects of an 
application, from web interface to map appearance can be developed 
without any programming. For the more ambitious user, MapServer 
applications can be enhanced using Python, PHP, Java, JavaScript or 
many other web technologies. For more  information and complete 
documentation please visit:

  https://mapserver.org/

Bug reports and enhancement submissions can be reported in the MapServer 
issue tracker at the following url.   If you do make changes and/or enhancements, 
please let us know so that they might be incorporated into future releases.

  https://github.com/MapServer/MapServer/issues


Join the MapServer user mailing list online at:

  https://mapserver.org/community/lists.html

 

Credits
-------

MapServer was originally written by Stephen Lime. Major funding for development of 
MapServer has been provided by NASA through cooperative argreements with 
the University of Minnesota, Department of Forest Resources.

PHP/MapScript developed by DM Solutions Group.

GDAL/OGR support and significant WMS support provided by DM Solutions Group 
which received funding support from Canadian Government's GeoConnections 
Program and the Canadian Forest Service.

Raster support developed by Pete Olson of the State of Minnesota, Land 
Management Information Center, and maintained by Frank Warmerdam (DM 
Solutions).

PostGIS spatial database support provided by Dave Blasby of Refractions 
Research.

PDF support developed by Jeff Spielberg and Jamie Wall of Market Insite Group, 
Inc.

OracleSpatial support developed by Rodrigo Cabral of CTTMAR/UNIVALI, Brazil.

Portions Copyright (c) 1998 State of Minnesota, Land Management Information 
Center.

Portions derived from Shapelib, Copyright 1995-1999 Frank Warmerdam.

Supporting packages are covered by their own copyrights.

License
-------

::

  Copyright (c) 2008-2022 Open Source Geospatial Foundation.
  Copyright (c) 1996-2008 Regents of the University of Minnesota.

  Permission is hereby granted, free of charge, to any person obtaining a copy 
  of this software and associated documentation files (the "Software"), to deal 
  in the Software without restriction, including without limitation the rights 
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
  copies of the Software, and to permit persons to whom the Software is furnished
  to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all 
  copies of this Software or works derived from this Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
  SOFTWARE.


.. |Build Status| image:: https://travis-ci.com/MapServer/MapServer.svg?branch=main
   :target: https://travis-ci.com/MapServer/MapServer

.. |Appveyor Build Status| image:: https://ci.appveyor.com/api/projects/status/vw1n07095a8bg23u?svg=true
   :target: https://ci.appveyor.com/project/mapserver/mapserver

.. |Coveralls Status| image:: https://coveralls.io/repos/github/MapServer/MapServer/badge.svg?branch=main
   :target: https://coveralls.io/github/MapServer/MapServer?branch=main

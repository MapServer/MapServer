MapServer Revision History
==========================

This is a human-readable revision history which will attempt to document
required changes for users to migrate from one version of MapServer to the
next.  Developers are strongly encouraged to document their changes and
their impacts on the users here.  (Please add the most recent changes to
the top of the list.)

For a complete change history, please see the Git log comments.  For more
details about recent point releases, please see the online changelog at:
https://mapserver.org/development/changelog/

The online Migration Guide can be found at https://mapserver.org/MIGRATION_GUIDE.html

8.4.0 release (2025-01-15)
--------------------------

- add CITATION.cff, useful for Zenodo/DOI (#7209)
(see major changes below)

8.4.0-rc1 release (2025-01-08)
------------------------------

- include stdbool.h in mapserver.h (#7205)

8.4.0-beta2 release (2024-12-14)
--------------------------------

- add explicit error message when proj.db cannot be found (#7203)

8.4.0-beta1 release (2024-12-13)
--------------------------------

- add PCRE2 support (#7073)

- add CONNECTIONTYPE RASTERLABEL (#7135)

- set MS_LEGEND_KEYSIZE_MAX to 1000 (#7154)

- add 4 new COMPOSITE.COMPOP blending operations (#7065)

- allow encryption key files to use paths relative to a mapfile (#7181)

- allow use_default_extent_for_getfeature to be used for OGC Features API and PostGIS (#7197)

see detailed changelog for other fixes

8.2.2 release (2024-09-02)
--------------------------

- fix build against FreeType 2.13.3 release (#7142)

see detailed changelog for other fixes

8.2.1 release (2024-07-21)
--------------------------

- security: validate tostring() expression function (#7123)

- handle PHP MapScript out of source builds (#7108)

7.6.7 release (2024-07-21)
--------------------------

- security: validate tostring() expression function (#7123)

8.2.0 release (2024-07-08)
--------------------------

RC3 was released as the final 8.2.0 (see major changes below)

8.2.0-rc3 release (2024-06-28)
------------------------------

- fix memory corruption in msUpdate...FromString (#7038)

8.2.0-rc2 release (2024-06-27)
------------------------------

- fix SWIG MapScript build failure (#7090)

8.2.0-rc1 release (2024-06-14)
------------------------------

- no changes since beta3 (see major changes below)

8.2.0-beta3 release (2024-06-11)
--------------------------------

- security fix to prevent SQL injections through regex validation (#7075)

7.6.6 release (2024-06-11)
--------------------------

- security fix to prevent SQL injections through regex validation (#7075)

8.2.0-beta2 release (2024-06-08)
--------------------------------

- fix Python MapScript installation (#7071)

8.2.0-beta1 release (2024-06-06)
--------------------------------

- restructure repo to move source code into /src (#6837)

- remove sym2img.c from codebase (#6593)

- handle templates + includes (#6113)

- handle empty query response (#6907)

- remove support for GDAL < 3 (#6901)

- remove support for PROJ < 6 (#6900)

- OGC API Features: implement Part 2 - CRS (#6893)

- allow custom projections to be requested via WMS (#6795)

- allow mapfiles to be accessed via URL Keys (#6862)

- allow expressions in LABEL PRIORITY (#6884)

- reference SLD files in Mapfiles (#7034)

see detailed changelog for other fixes

8.0.1 release (2023-04-17)
--------------------------

- fix WFS paging on Oracle (#6774) 

- allow runtime substitutions on the Web template parameter (#6804)

- handle multiple PROJ_DATA paths through config (#6863)

see detailed changelog for other fixes

8.0.0 release (2022-09-12)
--------------------------

RC2 was released as the final 8.0.0 (see major changes below)

8.0.0-rc2 release (2022-09-05)
------------------------------

- fix !BOX! token not working for a PostGIS connection (#6601)

- check if a LAYER has a NAME, before including it in a GROUP (#6603)

8.0.0-rc1 release (2022-08-19)
------------------------------

- add missing include to fix build issue on some compilers (#6595)

- update the distributed mapserver-sample.conf (#6598)

8.0.0-beta2 release (2022-08-09)
--------------------------------

- install coshp utility (#6540)

- improve error messages about missing mandatory metadata (#6542)

- fix reprojection of lines crossing the antimeridian (#6557) 

- config file parsing: use msSetPROJ_LIB() when ENV PROJ_LIB is set (#6565)

- handle PROJ_DATA as well as PROJ_LIB (#6573)

- reset layer filteritem to its old value in case of no overlap (#6550)

8.0.0-beta1 release (2022-06-27)
--------------------------------

- add new MapServer config file requirement (RFC 135)

- initial OGC API support (RFC 134)

- rename shp2img utility to map2img (RFC 136)

- make STYLES parameter mandatory for WMS GetMap requests (#6012)

- improve SLD label conformance (#6017)

- enable PHP 8 MapScript support, through SWIG, re-enable unit tests, and remove old native PHP MapScript (#6430) 

- remove deprecated mapfile parameters (RFC 133)

- improve numerical validation of mapfile parameter values (#6458)

- fix various security vulnerabilities found by libFuzzer (#6419)

- add new GEOMTRANSFORM 'centerline' labeling method for polygons (#6417)

- upgrade Travis and GitHub CI to run on Ubuntu Focal (#6430)

7.6.5 release (2023-04-17)
--------------------------

- remove password content from logs (#6621)

- increase security and stability (#6818)

see detailed changelog for other fixes

7.6.4 release (2021-07-12)
--------------------------

- improved performance of GPKG and SpatiaLite queries (#6361)

- WFS: fix paging with GPKG/Spatialite datasources and non-point geometries (#6325)

- PostGIS: use ST_Intersects instead of && for bounding box (#6348)

see detailed changelog for other fixes

7.6.3 release (2021-04-30)
--------------------------

- fix security flaw for processing the MAP parameter (#6313)

- fix code defects through Coverity Scan warnings (#6307)

- add support for PROJ 8 (#6249)

see detailed changelog for other fixes

7.6.2 release (2020-12-07)
--------------------------

- No major changes, see detailed changelog for bug fixes

7.6.1 release (2020-07-31)
--------------------------

- No major changes, see detailed changelog for bug fixes

7.6.0 release (2020-05-08)
--------------------------

- for essential WMS client layers set EXCEPTIONS to XML by default (#6066)

7.6.0-rc4 release (2020-05-04)
------------------------------

- Fix case insensitive 'using unique' for PostGIS connections (#6062)

- Add handling essential WMS layers (#6061)

7.6.0-rc3 release (2020-04-24)
------------------------------

- Handle special characters in paths on Windows (#5995)

- Add attribute binding for label ALIGN, OFFSET, POSITION (#6052)

7.6.0-rc2 release (2020-04-10)
------------------------------

- Fix memory corruption in msGEOSGetCentroid (#6041)

7.6.0-rc1 release (2020-04-05)
------------------------------

- Fix alpha value for hex colors (#6023)

7.6.0-beta2 release (2020-03-28)
--------------------------------

- fixed build with PHPNG + gnu_source (#6015)

- fixed rendermode with geomtransform (#6021)

7.6.0-beta1 release (2020-03-22)
--------------------------------

- Full support for PROJ6 API (#5888)

- Enable PointZM data support (X,Y,Z,M coordinates) by default

- SLD support enhancements (RFC 124) except PR#5832

- CONNECTIONOPTIONS support in mapfile LAYER (RFC 125)

- Enforce C99 and C++11 build requirements (RFC 128)

7.4.4 release (2020-3-20)
-------------------------

- Security release, see ticket #6014 for more information.

7.4.3 release (2019-12-16)
--------------------------

- No major changes, see detailed changelog for bug fixes

7.4.2 release (2019-9-13)
-------------------------

- No major changes, see detailed changelog for bug fixes

7.4.1 release (2019-7-12)
-------------------------

- No major changes, see detailed changelog for bug fixes

7.4.0 release (2019-5-14)
-------------------------

- No major changes, see detailed changelog for bug fixes

7.4.0-rc2 release (2019-5-10)
----------------------------

- No major changes, see detailed changelog for bug fixes

7.4.0-rc1 release (2019-5-1)
----------------------------

- No major changes, see detailed changelog for bug fixes

7.4.0-beta2 release (2019-4-17)
-------------------------------

- Addresses XSS issue with [layers] template tag (fix available in 6.4, 7.0 and 7.2 branches as well)

- Added Perl/Mapscript to Travis CI

- No other major changes, see detailed changelog for bug fixes

7.4.0-beta1 release (2019-3-29)
-------------------------------

- Python MapScript binding is available as installable Wheels with a full test suite and examples

- C# MapScript binding is now compatible with .NET Core

- PHP 7 MapScript binding support - both PHP/MapScript and Swig/MapScript

- Added workaround to allow compiling against Proj 6 (#5766)

7.2.2 release (2019-2-19)
--------------------------

- No major changes, see detailed changelog for bug fixes

7.2.1 release (2018-10-1)
--------------------------

- No major changes, see detailed changelog for bug fixes

7.2.0 release (2018-07-23)
--------------------------

- Fixed issue with ring handling with polygons in MVT support (#5626)

- No other major changes, see detailed changelog for bug fixes

7.2.0-beta2 release (2018-6-13)

- Update beta1 release notes to remove reference to PHP7 support

- No other major changes, see detailed changelog for bug fixes

7.2.0-beta1 release (2018-5-9)
------------------------------

- Support for Enhanced Layer Metadata Management (RFC82)

- Calculate MINDISTANCE from label bounds instead of label center (#5369)

- Reposition follow labels on maxoverlapangle colisions (RFC112)

- Implement chainable compositing filters (RFC113)

- Faster retrieval of shape count (RFC114)

- WMS layer groups are now requestable

- Support C-style multi-line content types (#5362)

- Python 3.x support (#5561)

- Support Vendor-Specific OGC FILTER parameter in WMS requests (RFC118)

- Mapbox Vector Tile (MVT) Support (RFC119)

- INSPIRE download service support for WCS 2.0 (RFC120)

7.0.0 release (2015/07/24)
--------------------------

- No major changes, see detailed changelog for bug fixes

7.0.0-beta2 release (2015/06/29)
--------------------------------

- No major changes, see detailed changelog for bug fixes

7.0.0-beta1 release (2015/02/12)
--------------------------------

- RFC91 Layer Filter Normalization

- Implement WCS20 Extensions (#4898)

- Require validation of ExternalGraphic OnlineResource (#4883)

- Require validation on the CGI queryfile parameter. (#4874)

- Apply RFC86 scaletoken substitutions to layer->PROCESSING entries

- RFC113 Layer Compositing Pipeline

- RFC109 Optimizing Runtime Substitutions

- RFC108 Heatmap / Kernel-Density Layers

- RFC106 Support of Geomtransform JavaScript Plugin

- RFC105 Support for WFS 2.0 (server side)

- RFC104 Bitmap Label removal, replaced with inlined truetype font

- RFC103 Layer Level Character Encoding

- RFC102 Support of Styleitem JavaScript Plugin

- RFC99 GD removal

- RFC98 Complex Text Rendering / Text Rendering Overhaul

- RFC93 UTF Grid Support

6.4 release (2013/09/17)
---------------------------

- RFC 101: Support for Content Dependent Legend Rendering

- Add Support for librsvg as an alternative to libsvg-cairo

- RFC100: Add support for raster tile index with tiles of mixed SRS (TILESRS keyword)

- RFC94: Shape Smoothing

- RFC95 : add EXPRESSION {value1,value2,...} support to parser (#4648)

- RFC92: Migration from autotools to cmake (#4617)

- RFC88: Saving MapServer Objects to Strings (#4563)

- RFC90: Enable/Disable Layers in OGC Web Services by IP Lists

- RFC85,89 Added Simplify,SimplityPT and Generalize geomtransform, implementation of
  geomtransform at the layer level

- RFC86: Scale-dependant token replacements (#4538)

- Fix symbol scaling for vector symbols with no height (#4497,#3511)

- Implementation of layer masking for WCS coverages (#4469)

- Implementation of offsets on follow labels (#4399)

6.2.0 release (git branch-6-2) 2012/11/14:
------------------------------------------

- Fix WFS GetFeature result bounds are not written in requested projection (#4494)

- Fixed wrong size in LegendURL of root layer (#4441)

- Fixed wms_style_name in GetMap requests (#4439)

- Fix segfault in queryByFilter function in MapScript

- Adjusted WCS 2.0 to WCS Core 2.0.1 and GMLCOV 1.0.1 corrigenda (#4003)

- Adjusted mediatype to multipart/related in WCS 2.0 (#4003)

- Fix symbolObj.getImage seg fault in PHP/MapScript

- Fix bad handling of startindex in some drivers (#4011)
  WFSGetFeature also now uses a global maxfeatures and startindex to support
  maxfeatures across layers rather than within layers

- Fixed bugs with WCS 1.1/2.0 with VSIMEM (#4369)

- Add notable changes here. Less important changes should go only in the
  commit message and not appear here.
  
- Fixed the OGR driver to use point or line spatial filter geometries in degenerated cases (#4420)

- implement OFFSET x -99 on ANGLE FOLLOW labels (#4399)

Version 6.2 (beta1: 20120629): 
------------------------------

- Fix WFS filter is produced as non-standard XML (#4171)

- Fix pixmap symbol loading issue (#4329)

- Added INSPIRE ExtendedCapabilities and DOCTYPE definition to WMS 1.1.1 (#3608)

- Fix wms_enable_request-related errors are not properly tagged (#4187)

- Fixed "msGetMarkerSize() called on unloaded pixmap symbol" in mapsymbol.c (#4225)

- Fixed PHP MapScript support for PHP 5.4 (#4309)

- Fix msOGREscapeSQLParam could return random data (#4335)

- Fixed locations of supported and native formats in Capabilities and CoverageDescriptions for WCS 2.0 (#4003)

- Made format parameter for WCS 2.0 GetCoverage requests optional (#4003)

- Swig mapscript for multi-label support (#4310)

- Fix creation of a vector symbolObj in mapscript (#4318)

- Added coverage metadata in WCS (#4306)

- Ignore unknown requets parameteres in WCS 2.0 (#4307).

- Fixed getFeature request with custom output format fails on filter encoding (#4190)

- Fixed resolution when UoM changes in WCS 2.0 (#4283)

- Added missing DEFRESOLUTION parameter to msCopyMap() function (#4272) 

- Migrated svn to git, and issue tracking from trac to github

- Fixed mapscript is unusable in a web application due to memory leaks (#4262)
 
- Fixed legend image problem with annotation layers with label offsets (#4147)

- Add support for multiple labels per feature (RFC81)

- Add support for INSPIRE view service (RFC 75)

- Drop support for GDAL/OGR older than 1.5.0 (#4224)

- PDF backend: add support for generating geospatial PDF, when
  GDAL 2.0 with PDF driver is used, and the GEO_ENCODING FORMATOPTION
  (set to ISO32000 or OGC_BP) is added to the OUTPUTFORMAT definition. (#4216)

- Python mapscript: fix swig mappings to work with both python 2.5.and 2.6 (#3940)

- Added classgroup CGI parameter support (#4207)

- Java mapscript: renamed shared library and completed libtool support (#2595)

- Fixed WCS 2.0 axis order (#4191)

- Added MS_CJC_* constants in PHP/MapScript (#4183)

- Fixed Memory Leak with Fribidi Strings (#4188)

- Allow multiple label definitions within a class (RFC 77/#4127)

- shp2img: make it possible to specify layers (with -l option) that match GROUP names.

- reuse a pre-parsed mapfile across fastcgi loops in case the mapfile is
  specified with the MS_MAPFILE env variable. (#4018)

- Ability to do use PropertyIsLike with a column of a different type than text (postgis) (#4176)

- Fixed SLD with FILTER doesn't apply properly to database layers (#4112)

- Fixed lexer buffer size issue with single quotes in a string  (#4175)

- WFS Quote Escape issue (#4087)

- Raster layer fails to be drawn if the window is less than half a pixel (#4172)

- shptree: Improvement to reduce size of .qix files (#4169)

- avoid potential gd fontcache deadlock on fastcgi exit signals(#4093)

- Adjusted WCS GetCapabilities for an empty list of layers (#4140)

- Adjusted WMS GetCapabilities for an empty list of layers (#3755)

- Refactor cgi masperv to not call exit on argument errors (#3099)

- Add --with-apache-module configure option to build an apache dso module
  (#2565)

- Use libtool to build object files and libraries

- Dynamically link libmapserver to the created binaries by default
  (requires the make install step to be run)

- Python mapscript builds with make instead of setuptools

- Fix LABELPNT geomtransform position for non-cached labels (#4121)

- Add RFC80 font fallback support (#4114)

- Added POLAROFFSET style option for a different symbol transform (#4117)

- automatically translate vector symbol points (#4116)

- Add RFC79 Layer masking (#4111)

- Fixed single pixel coverages in WCS 2.0 (#4110)

- Add svg symbols support (#3671)

- Fixed subsetting in WCS 2.0 (#4099)

- Upgrade clipper library to 4.6.3

- Make openlayers wms template request images with mimetype of outputformat
  defined in the mapfile's imagetype

- Fix memory leak in msSLDParseRasterSymbolizer()

- Fix compilation error with clang in renderers/agg/include/agg_renderer_outline_aa.h

- Add stable hatch rendering (#3847)

- Added vector field rendering (#4094)

- Add wms dimensions support (#3466)

- Fixed segfault when calling classObj::updateFromString() with SYMBOL (#4062)

- Use a renderer native follow text implementation if available.

- Fixed layer with inline feature to support multiple classes (#4070)

- Add support for rfc45 anchorpoint on marker symbols (#4066)

- Add initial gap support for line marker symbols (#3879)

- Fix center to center computation of gaps for line marker symbols
  and polygon tile patterns (#3867)

- Add initial gap support for line patterns (#3879)

- Fixed grid of thin lines in polygon symbol fills (#3868)

- Fixed cannot add a style to a label in PHP/SWIG Mapscript (#4038)

- Fixed schema validity issue for WCS 1.1 GetCoverage responses (#4047)

- remove default compiler search paths from the GD CFLAGS/LDFLAGS (#4046)

- Fixed Python Mapscript does not write COLOR to reference map (#4042)

- Added XMP support for metadata embedding, RFC 76, (#3932)

- Added GetLegendGraphic Cascading support (#3923)

- Rewrite  postgres TIME queries to take advantage of indexes (#3374)

- Unified OWS requests and applied to WCS (defaults to version 2.0 now) (#3925)

- WCS 1.0: Fix crash with some _rangeset_axes values (#4020)

- WCS 2.0: Adjusted offset vector and origin (#4006)

- Added addParameter() method to MapScript (#3973)

- Changed msDrawVectorLayer() not to cache shapes if attribute binding is present after	the first style	(#3976)

- Fix mapscript to build when TRUE macro is not defined (#3926)

- Fix mapscript php build issues with MSVC (#4004)

- PHP MapScript is missing many styleObj properties (#3901)

- PHP/Mapscript: Segmentation fault when getting complex object using PHP 5.2  (#3930)

- PHP/Mapscript: Fixed webObj->metadata returns a webObj (#3971)

- WCS 1.1: Added check for imageCRS in msOWSCommonBoundingBox() (#3966)

- Fixed contains operator in logical expressions (#3974)

- WCS 1.0: WCS Exceptions raise mapscript exceptions (#3972)

- WMS: LAYERS parameter is optional when sld is given (#1166)

- Add runtime substitution for "filename" output format option (#3571) and
  allow setting defaults in either metadata or validation (preferred) blocks
  for both layer and output format.

- Add non antialiased text rendering for GD (#3896)

- Fixed OGC filter using expressions (#3481)

- Fix incorrect rendering of GD lines between 1 and 2 pixels wide (#3962)

- Add gamma correction to AGG polygon rendering (#3165)

- Initialize the scalebar image color to transparent by default (#3957)

- Do not divide by zero in io read/write funcs (#4135)

IMPORTANT SECURITY FIX:

-  Fixes to prevent SQL injections through OGC filter encoding (in WMS, WFS 
   and SOS), as well as a potential SQL injection in WMS time support. 
   Your system may be vulnerable if it has MapServer with OGC protocols 
   enabled, with layers connecting to an SQL RDBMS backend, either 
   natively or via OGR (#3903)

- Fix performance issue with Oracle and scrollable cursors (#3905)

- Fix attribute binding for layer styles (#3941)

- Added missing fclose() when writing query files (#3943)

- Fix double-free in msAddImageSymbol() when filename is a http resource (#3939)

- Fix rendering of lines with outlinewidth set if not on first style (#3935)

- Correct SLD with spatial filters bbox and intersects (#3929)

- Applied patch for ticket (symbol writing issues) (#3589)

- Added WMS GetFeatureInfo Cascading (#3764)

- Fixed png lib is not found on multiarch systems (#3921)

- Fixed PHP MapScript opacity property of StyleObj no longer works (#3920)

- Fixed Using STYLEITEM AUTO, loadExpression fails when the label text
  contains a space or begins with a double quote (#3481)

- Fix for the cluster processing if the shape bounds doesn't overlap 
  with the given extent (#3913)
  
- Add support for dashes on polygon hatches (#3912)

- Union layer 3 new processing options added (#3900)

- Changed msRemoveStyle to allow removing all styles (#3895)

- Fixed mssql2008 to return correct geometries with chart layer type (#3894)

- Ensure msLayerSetProcessingKey() supports setting a NULL value to clear
  a processing key. 

- Write SYMBOLSET/END tags when saving a symbol file (#3885)

- Make java threadtests work again (#3887)

- Fix segfault on malformed <PropertyIsLike> filters (#3888)

- Fixed the query handling problem with the Oracle spatial driver (#3878)

- Fixed potential crash with AVERAGE resampling and crazy reprojection (#3886)

- Adjusted OperationsMetadata for POST in WCS 2.0 GetCapabilities response

- Fix for the warnings in mapunion.c (#3880)

- SLD: correct when same layer is used with multiple styles (#1602)

- Fixed the build problem in mapunion.c (#3877)

- Implement to get all shapes with the clustered layer (#3873)

- Union layer: Fixed the crash when styling source layers using attributes (#3870)

- Added GEOS difference operator to expression support (#3871) 

- Improve rangeset item checking so that Bands=1,2,3 is supported with WCS 1.0
  (#3919).

- Fix GetMapserverUnitUsingProj() for proj=latlong (#3883)

- Add support for OGR output of 2.5D geometries (#3906)


Version 6.0.0 (2011-05-11)
--------------------------

- apply fix for #3834 on legend icon rendering (#3866)

- Union layer: Fixed a potential seg fault in msUnionLayerNextShape (#3859)

- Cluster layer: Fixed the problem when returning undefined attribute (#3700)

- Union layer: Fix for the item initialization at the source layer (#3859)

- Union layer: Fixed the potential seg fault when STYLEITEM AUTO is used (#3859)

Version 6.0.0-rc2 (2011-05-05)
------------------------------

- Fixed seg fault with [shpxy] tag... (#3860)

- Removed obsolete msQueryByOperator() function

- Call msLayerClose() before msLayerOpen() in the various query 
  functions (#3692)

- Fix WMS 1.3.0 to use full list of epsg codes with inverted axis (#3582)

- PHP/Mapscript: Added getResultsBounds() method in layer object (#2967)

- Fix SLD containing a PropertyIsLike filter (#3855)

- Fixed msUnionLayerNextShape to return correct values (#3859)  

- Union layer: fix for the failure if one or more of the source layers 
  are not in the current extent (#3858)

- Fix memory leak in msResampleGDALToMap() (#3857)

- Fix missing initialization of default formats in WCS 1.x.

- Fix maxoverlapangle when value is set to 0 (#3856)

Version 6.0.0-rc1 (2011-04-27)
------------------------------

- Fix for the styleitem handling with union layer (#3674)

- Fixed mindistance label test to check layer indexes. (#3851)

- Fixed segmentation fault in PHP/MapScript and improved the php object
  method calls (#3730)

- Fix build issue related to unnecessary use of gdal-config --dep-libs (#3316)

Version 6.0.0-beta7 (2011-04-20)
--------------------------------

- Union Layer: fix for the STYLEITEM AUTO option (#3674)

- Union Layer: Add support for the layer FILTER expressions, 
  add "Union:SourceLayerVisible" predefined attribute (#3674)

- fix circle layer drawing for edge case when point1.x==point2.x (#1356)

- fix incorrect quantization for images with very large number of
  colors (#3848)

- fix poor performance of polygon hatching (#3846)

- upgrade clipper library to 4.2 (related to #3846)

- Fix configure output for "WFS Client" (was reporting WMS info, #3842)

- KML: latlon bbox for raster layers could end up being wrong for non-square
  requests (#3840)

Version 6.0.0-beta6 (2011-04-13)
--------------------------------

- define EQUAL/EQUALN macros if cpl_port.h was not included (#3844)

- add configurable PNG/ZLIB compression level (#3841)

- SLD: use pixmap size when parameter size is not specified (#2305)

- fix memory leaks in mapgraticule.c (#3831)

- fix runtime sub validation against web metadata, was using wrong lookup key

- clean up the symbolset if we've used an alternate renderer for a 
  layer (#3834,#3616)

- fix crash on embedded legend with cairo raster renderer

- fix crashes in SVG renderer on polygon symbol fills (#3837)

- fix crash/corruptions with raster layers in pdf outputs (#3799)

- fix memory leak in msFreeLabelCacheSlot (#3829)

- use a circle brush for wide GD lines (#3835)

- fix segmentation fault with transparent layers containing symbols (#3834)

- fix memory leak on tiled vector polygons

- fix segfault with marker symbols on short lines (#3823)

- wms_getmap_formatlist causes first defined outputformat to be returned by 
  getmap (#3826)

- fix building of mapcluster.c when OGR support is disabled

- fix some valgrind found memory leaks (offset symbols, and gd io contexts)

- skip marker symbol with no defined SYMBOL (caused some memory leaks with
  uninitialized vector points)

- fix crash in GD lines with floating point dash patterns (#3823)

- Check renderer before using it when calculating label size (#3822)

- allow palette file path to be relative to mapfile (#2115)

- use supplied offset for brushed lines (#3825, #3792)

- fix division by 0 error in bar charts for some ill-defined cases (#3218)

- add GAP, POSITION and CAPS/JOINS to mapfile writer (#3797)

- fix GEOMTRANSFORM rotation orientation for vector symbols (#3802)

- GD Driver broken in FastCGI (#3813)

- configure: look for libexslt.so under lib64 as well

- Coding style and formatting fixes (#3819, #3820, #3821, and more)

- More improvements to OpenGL error handling (#3791)

- Added WMS GetFeatureInfo RADIUS=bbox vendor-specific option (#3561)

Version 6.0.0-beta5 (2011-04-06)
--------------------------------

- Fix setting of top-level mapObj member variables in PHP MapScript (#3815)

- More robust OpenGL context creation on older video cards and drivers (#3791)

- Allow users to set the maximum number of vector features to be drawn (#3739)

- Fixed FCGI on Windows problem related to lexer (#3812)

- KML: Add ows/kml_exclude_items (#3560)

- Removed all refs left to MS_SYMBOL_CARTOLINE (#3798)

- Removed GAP, PATTERN, LINECAP/JOIN and POSITION from symbolObj (#3797)

- Fixed handling of STYLEITEM AUTO label position codes 10,11,12 (#3806)

- Fixed msGEOSGeometry2Shape to handle 'GEOMETRYCOLLECTION EMPTY' 
  as null geometry instead of raising an error (#3811)

- Re-added the MYSQL JOIN support. Had been removed with the MYGIS 
  deprecated driver.

- Add opacity to legend (#3740)

- Updated PHP/MapScript with the new objects properties (#3735)

- KML: set layer's projection when it is not defined (#3809)

- Updated xml mapfile schema and xsl with the new lexer properties (#3735)

- Updated PHP/MapScript for MS RFC 69: clustering. (#3700)

- Move allocation of cgiRequestObj paramNames/Values to msAllocCgiObj() (#1888)

- Add support for simple aggregates for the cluster layer attributes (#3700)

- Improved error reporting in msSaveImage() (#3733)

- configure: look for libxslt.so under lib64 as well

- added missing ';' before charset in WFS DescribeFeatureType header (#3793)

- add brushed line support for agg renderer (#3792)

- fix bug with marker symbols along offset line

- fix for the cluster layer returning invalid feature count (#3794)

- remove some compiler warnings

- fix incorrect scaling of hatch symbol spacing (#3773)

- fix incorrect background color for INIMAGE exceptions (#3790)

Version 6.0.0-beta4 (2011-03-30)
--------------------------------

- Fix shp2img's -i flag to honour map level transparent, image quality and 
  interlace settings.

- Make sure command-line programs use an exit status other than 0 
  when an error is encountered. (#3753)

- Applied patch to filter unwanted fribidi characters (#3763)

- Fixed lexer to set the proper state on URL variable substitutions

- Fixed Memory leak in PostGIS driver (#3768)

- Fixed PHP/MapScript symbol property setter method

- fix memory leak in bar charts

- fix some valgrind errors on agg rotated pixmap symbols

- WCS 2.0: Adjusted definition of NilValues.

- Fixed handling of PROJ_LIB value relative to mapfile path (#3094)

- Fixed compilation error with opengl support (#3769)

- add support for gml:Box for spatial filters (#3789)

- fix query map generation error introduced in beta2 when output format 
  initialized was made as needed. 

- fix incorrect PATTERN behavior on agg lines (#3787)

- report SOS DescribeObservationType in Capabilities (#3777)

- Updated lexer to detect time attribute bindings (e.g. `[item]`) in logical
  expressions

- Fixed layer context expressions (REQUIRES/LABELREQUIRES) (#3737)

- KML: Output ExtendedData (#3728)

- Fixed OWS usage of multiple layers with same name (#3778)

- WCS implementation should not lookup wms_* metadata (#3779)

- WCS 2.0: Adjusted definition of rangeType.

- Fixed OWS GetCapabilities to report only requests/operations that are 
  enabled.

- WCS 2.0: Report only bands in the range subset.

- OWS requests should be completely blocked by default (#3755)

- SLD: check for limit on dash arrays (#3772)

- WMS: Apply sld after bbox and srs have been parsed (#3765)

Version 6.0.0-beta3 (2011-03-23)
--------------------------------

- apply min/max size/width style values to polygon spacing (vaguely related 
  to #3132)

- assure that a created tile has a non-zero width and height (#3370)

- use png_sig_cmp instead of png_check_sig (#3762)

- Rendering: scale style OFFSET and GAP the same way we scale other style 
  attributes. Beforehand, we scaled them proportionaly to the computed width.

- KML: fix rounding problem for point feautres (#3767)

- KML: update code to reflect output changes. Fix true type symbols. (#3766)

- SLD: Text Symbolizer now uses the new expression syntax (#3757)

- WFS: correct bbox values for GetFeature with featureid request (#3761) 

- Mapscript Seg Fault on mapObj->getMetaData (#3738)

- Correct double free in msCleanup().

- Initialize default formats in WCS.

- Fix csharp Makefile.in (#3758)

- Allow run-time subs in class->text (makes sense if you allow it in 
  class->expression).

- Fix build problem when --enable-cgi-cl-debug-args is enabled (#2853)

Version 6.0.0-beta2 (2011-03-18)
--------------------------------

- correct scaling of symbol GAP and PATTERN (#3752)

- remove references to SWF/MING

- CGI runtime substitution requires a validation pattern (was optional 
  before) (#3522)

- add a default png8 outputformat that uses AGG/PNG with quantization

- change MS_INIT_COLOR to take alpha as a parameter

- stop using style->opacity in rendering code, use alpha from colorObjs.

- Fixed big Oracle memory leak when rendering in KML (#3719)

- avoid linking in postgres dependencies unnecessarily (#3708)

- don't initialize outputformats until they are selected

- use "seamless" creation of tiles for polygon fills with vector symbols

- Ability to escape single/double quotes inside a string (#3706)

- Globally replace msCaseFindSubstring with strcasestr (#3255)

- support GROUP layers in shp2img (#3746)

- Honour MAXSIZE for WCS 2.0 responses (#3204).

- fallback to ows_title for WCS ows:Title of CoverageDescription (#3528)

- Added msIO_stripStdoutBufferContentHeaders() to strip off all 
  Content-* headers from a buffer (#3673, #3665).

- Added raster classification support for STYLE level OPACITY.

- Allow attribute references, that is [itemname], within a TEXT string (#3736)

- Fixed segmentation fault when parsing invalid extent arguments in 
  shp2img (#3734)

- Make "openlayers mode" work even without OWS support (#3732)

- Add a static table to define the axis order for some epsg codes (#3582)

- Add possibility to use KML_NAME_ITEM (#3728)

- Fixed mapfile parsing error when a label angle referenced an attribute 
  (e.g. ANGLE [angle]) #3727

- Removed executable flag on some source files (#3726)

- Fixed SQL Spatial to be able to use UniqueIdentifier field as unique 
  key (#3722)

- Fix PHP Windows build (#3714)

- Fixed --with-opengl build issue: Look for OpenGL libs under /usr/lib64 as 
  well (#3724)


Version 6.0.0-beta1 (2011-03-09)
--------------------------------

- Fixed Core Dump from Format=KML/Z with Oracle Spatial layers (#3713)

- Call msPluginFreeVirtualTableFactory from msCleanup (#2164)

- Avoid the crash if anglemode 'FOLLOW' is specified with bitmap fonts. (#3379)

- Add argument check for processTemplate, processLegendTemplate and 
  processQueryTemplate in the C# bindings (#3699)

- Remove shapeObj.line, shapeObj.values, lineObj.point from the SWIG API 
  which are redundant and undocumented. (#3269)

- Remove map-oriented query modes (e.g. QUERYMAP). Use qformat parameter instead. (#3712)

- Implement single-pass query handling in mssql2008 driver as per RFC 65.

- Fixed Sql Server 2008 key column name truncation (#3654)

- Added label position binding (#3612) and label shadow size binding (#2924)

- Implement MS RFC 69: Support for clustering of features in point layers (#3700)

- Implement MS RFC 68: Support for combining features from multiple layers (#3674)

- Add support for WCS 1.1 Post (#3710)

- Fixed OGR query handling according to RFC 65 (#3647)

- Implement CLOSE_CONNECTION=ALWAYS to suppress the connection pooling
  for a particular layer.

- Implemented RFC 67: Enable/Disable layers in ogc web services (#3703)

- Class title can now be used in legends. It's value takes precedence over class name. Previously
  title was not used any place in the code but it is supported for read/write. (#3702)

- Allow definition of nodata attribute for layers without results (via resultset tag). (#3701)

- mapprojhack.c: restructure to avoid needing projects, or any internal 
  knowledge of PROJ.4.  

- Fix newlines around part boundaries in WCS multipart results (#3672)

- Added minfeaturesize support at layer or class level (#1340)

- Implemented support in for classifying rasters using the new
  expression parsing (msGetClass()...)  (#3663)

- Implemented RFC 65 which improves and simplifies one-pass query support. This causes
  a few MapScript regressions with getShape/getFeature/getResultsShape. (#3647)

- Support setting filenames for WCS GetCoverage results (#3665)

- OGR auto-styling: use the color parameter and set the style's opacity when it is available.
  Allow symbols that can be stored externally (#3660)

- Add possibility to use symbols stored externally, accessed through http (#3662)

- Better handling of temporary files (#3354)

- Support curved features in PostGIS (#3621)

- NODATA values now excluded from autoscaling min/max for non-eight 
  scaling computations (#3650)

- Fixed missing time in msDrawMap logging (#3651)

- Fixed Auto Angle - incorrectly placed Labels (#3648)

- Fixed Improper validation of symbol index values (#3641)

- Removed BACKGROUNDCOLOR, BACKGROUNDSHADOWCOLOR and BACKGROUNDSHADOWOFFSET label parameters (#3609)

- Fixed Transformation from XML to MapFile only handles one PROCESSING element (#3631)

- Fixed 16bit classification support - problem introduced with new 
  renderer architecture (#3624) 

- Cleanup open gdal datasets in msGDALCleanup() (if we have a very new
  GDAL).  This makes it easier to identify memory leaks. 

- Add support for per layer tuning of the shape to pixel conversion (SIMPLIFY, ROUND,
  SNAPTOGRID, FULLRESOLUTION)

- Fixed: Memory allocation results should always be checked (#3559)

- Fixed free(): invalid next size in mapfile.c (#3604)

- Added a built-in OpenLayers map viewer (#3549)

- Fixed issues with static buffers and sprintf (#3537)

- Fix for the memory corruption when mapping the string data type in the Java bindings (3491)

- Fixed double free in shp2img.c (#3497)

- Fixed number of CGI params is limited to 100 (#3583)

- Fixed duplicated XML and HTML errors from WFS GetFeature (#3571)

- Support group names for GetLegendGraphic and GetStyles (#3411)

- apply patch (thanks rouault) to advertise resultType=hits in WFS 1.1 Capabilities (#3575)

- mapshape.c: Fix writing of geometries with z/m and fail gracefully attempting
  to create such files if USE_POINT_Z_M is not enabled (#3564)

- sortshp.c: improve error handling (#3564)

- MSSQL2008: Add support for geography data type by extending the DATA syntax 
  to 'geometry_column(geometry_type) from table_name'

- Fixed ability to get the error message and code of an OWS exception (#3554)

- Fixed msOGRGetSymbolId according to the changes in gdal 1.8 (#3556)

- Support holding WMS client requests in RAM instead of writing to disk (#3555)

- RFC-61: Enhance MapServer Feature Style Support (#3544)

- Restrict cascaded WMS requests based on EXTENT (#3543)

- Avoid EPSG lookups for WMS client layers if possible (#3533)

- RFC-60: Add ability to skip ANGLE FOLLOW labels with too much character overlap (#3523)

- Fixed SWF: not a valid Bitmap FillStyle (#3548)

- Set dependency on libxml2 when building --with-wfs (#3512)

- Fixed TRUE is undefined in shptreevis (#3545)

- shptreevis: bug truncates visualization shapefile if there are more
  nodes in the tree than there are shapes being indexed! 

- Fixed multiple include tags not supported in xml mapfiles (#3530)

- Support reading .dbf files between 2GB and 4GB (#3514)

- Avoid warnings about ms_cvsid being unused with gcc. 

- Ensure the class is not marked BeforeFieldInit causing memory corruption with C#/CLR4 (#3438)

- Fixed MSSQL2008 driver returning invalid extent (#3498)

- Added coordinate scaling to shpxy tag via parameters scale, scale_x or scale_y.

- Fix computation of shape bounds when the first line contains no points
  (#3119)(fixes #3383)

- Allow map relative specification in the PROJ_LIB config item (#3094)

- Try to report reprojection errors in SLD Filter evaluation (#3495)

- Disabled some insecure (and potentially exploitable) mapserv command-line
  debug arguments (#3485). The --enable-cgi-cl-debug-args configure switch
  can be used to re-enable them for devs who really cannot get away without
  them and who understand the potential security risk (not recommended for 
  production servers or those who don't understand the security implications).

- Fixed segmentation fault in ogr driver when shape type is null 

- Fixed synchronized MS_UNITS and inchesPerUnits array (#3173) 

- Fixed possible buffer overflow in msTmpFile() (#3484)

- Fixed Using STYLEITEM AUTO, loadExpression fails when the label text contains a double quote (#3481)

- PHP/MapScript: Expose getGeomTransform/setGeomTransform instead of exposing the private vars for rfc48 (#2825) 

- PHP/MapScript: Fixed updateFromString functions to resolve symbol names (#3273)

- PHP/MapScript: ability to use the php clone keyword (#3472)

- Modified mapserver units enum order to fix some problems with external software (#3173)

- Fixed configure does not detect libGD version dependencies (#3478)

- Fixed Drawing inline text not working (bitmap) (#3475)

- ensure well formed XML when msWCSGetCapabilities_CoverageOfferingBrief
  returns MS_FAILURE (#3469)

- Fixed msQueryByRect does not use tolerance (#3453)

- Fixed MapScript processTemplate and processQueryTemplate seg fault (#3468)

- Fixed shapeObj->toWkt() returns single point for multipoint geometry (#2762)

- Fixed Internal server error when Oracle returns ora-28002 code (#3457)

- Fixed PHP/MapScript ms_iogetstdoutbufferbytes() always returning 0 bytes written (#3041)

- MapScript resultsGetShape() method fails with a OracleSpatial layer (#3462)

- PHP/MapScript circular references between objects (#3405) 

- Handle null results with gml:Null/gml:null according to version (#3299)

- Reworked mapfile writing to use helper functions so that core types (e.g. numbers, strings,
  colors, keywords, etc...) are always written consistently.

- Ensure mapwmslayer.c does not unlink file before closing connection on it (#3451)

- Fix security exception issue in C# with MSVC2010 (#3438)

- Write out join CONNECTIONTYPE when saving a mapfile. (#3435)

- Fixed attribute queries to use an extent stored (and cached) as part of the queryObj
  rather than the map->extent. (#3424)

- Reverted msLayerWhichItems() to 5.4-like behavior although still supporting
  retrieving all items (#3356,#3342)

- Grid layer: remove drawing of unnecessary gird lines (#3433) 

- Avoid errors and debug output for CONNECTION-less OGR layers in mappool.c
  if they have a tileindex.

- Implement support for raw imagemodes to use NULLVALUE formatoption to set 
  the background value of result images, and to try and mark nodata (#1709)

- Implement support for wcs_rangeset_nullvalue to set NULLVALUE and
  return null value definition in describe coverage result (#655)

- Try to avoid exhausting the color table when rendering 24bit key
  images into 8bit results (#1594)

- Avoid crash, and ensure error report when loading keyimage fails (#1594)

- Improve the handling of simple string comparisons for raster classified
  values (#3425) 

- Generate good SQL when using !BOX! token and no filter. (#3422)

- Implement non-shapefile tileindex support for raster query (#2796).

- Improve support for [red/green/blue] classification expressions for
  raster query (#1021)

- Fixed imageObj->saveImage() sends unnecessary headers (#3418)

- Avoid automatically regenerating maplexer.c (#2310)

- Change rounding rules for average resampling (#1932)

- Implement support for filename encryption per RFC 18 for rasters (#3416)

- Fixed segfault when using shapefile with empty geometry and tileindex (#3365)

- Avoid race condition on core_lock on win32 mutex init (#3396)

- Avoid use of inline keyword for C code (#3327)

- Support WFS parsing through libxml2 (#3364)

- Fixed PHP/MapScript imageObj->saveImage() function (#3410)

- Implement wms_nonsquare_ok metadata item for WMS servers that do
  not support non-square pixels (#3409)

- Fixed MapScript shape->classindex is always 0 (#3406)

- Fixed PHP MapScript integer passing broken on 64bit systems (#3412)

- Fix MS_NONSQUARE to work in mode=map (#3413)

- Support inclusion of raster layers in query map drawing even if the results
  may not be that useful (#1842).

- Fixed auto angle: incorrectly rotated Labels. Added AUTO2 angle mode. (#1688)

- Preliminary implementation of validity mask (imageObj->img_mask) for raw
  raster data (#2181)

- Added libgd < 2.0.30 compatibility (#3293)

- Incorporate support for GDAL nodata on RGB images (#2404)

- Incorporated support for GDAL masks (GDAL RFC 15) (#2640)

- Fixed XML transformation issues with expressions and symbols (#3397)

- Check error returns from mapstring functions (#2988)

- Add support for multiliple srs in  WFS 1.1.0 (#3227)

- Fixed PHP/MapScript owsRequestObj->loadParams() method when using php in a non cgi/fcgi/cli mode. (#1975)

- Ensure that non-file raster datasets work (#3253) 

- Correct mutex locking problem with rasters with no inherent georef. (#3368) 

- Correct ungeoreferenced defaults via GetExtent() on raster layer (#3368)

- PHP Mapscript refactoring: take full advantage of PHP 5 / Zend Engine 2 (#3278)"

- Fixed msRemoveHashTable() to return the proper value on failure/success.

- Correct one pass query problems and OGC filter query (#3305)

- Fixed msMSSQL2008CloseConnection() to free the statement handle properly (#3244)

- Fixed the query handling with the MSSQL2008 driver (#3058)

- Fixed swig zoomRectangle() method: the maxy in the rect object have to be < miny value (#3286)

- Fix crash with GRID layers with no classes (#3352)

- Remove "legacy" raster support, all raster rendering via GDAL now.

- Very preliminary render plugin support for raster rendering. (RFC 54)

- support correct MIME type output for WFS 1.1.0 (#3295)

- add WMS 1.3.0 LayerLimit support (#3284)

- fix WFS 1.1.0 exception attributes (#3301)

- add more useful error message when query file extension test fails (#3302)

- s/gml:null/gml:Null for empty WFS GetFeature responses (#3299)

- Support metatiling and buffers in mode=tile (#3323)

- Enhance error messages in msGetTruetypeTextBBox() (#3314)

- Report parsing errors in INCLUDEd files using the line number within the file (#3329)

- Avoid memory error when building SQL bbox (#3324)

- Reproject rectangles as polygons to get datelin wrapping (#3179)

- Add support for the WMS capabilities items AuthorityURL, Identifier (#3251)

- Added support to write a null shape in a shape file. (#3277)

- Apply ON_MISSING_DATA to zero-length WMS client calls (#2785, #3243)

- PHP/Mapscript: added labelCacheMember object and mapObj::getLabel() method (#1794)

- Add shplabel tag support in templates (#3241)

- Bumped GEOS requirement to version 3.0+ (#3215)

- Fixed memory leak related to templates (#2996)

- Added support of 44xx gtypes in oracle spatial driver (#2830)

- Fixed curl proxy auth support for http connections (#571)

- PHP/MapScript: removed deprecated class properties (#2170)

- Fixed OGR datasource double free (#3261)
 
- Fix compilation warnings around use of strcasestr (#3257)

- Made %substitution% strings case insensitive (#3250)

- Added support to get the extent of a raster layer that use a tileindex (#3252)

- Fixed configure to support FTGL 2.1.2 (#3247)

- Changed msSaveImageBufferGD to be in accordance with msSaveImageGD (#3201)

- PHP/Mapscript: added layerObj units property (#3249)

- Changed the query map rendering implementation without adding extra layers to the map (#3069)

- SQL Server 2008 plugin is not handling null field values correctly (#2893)

- Hatch symbol not properly saved (#2905)

- Expose symbolObj.inmapfile to the SWIG API, have already been exposed to PHP (#3233)

- Expose getGeomTransform/setGeomTransform to SWIG instead of exposing the private vars for rfc48 (#3214)

- Fixed writeSymbol to support writing 'ANGLE AUTO' (#3214)

- Fixed problems with point queries not working via the CGI (mode=query or mode=nquery) (#3246)

- Support QueryByShape() with point and line geometries (#3248)

- Honour MAXSIZE for WCS responses (#3204)

- Implemented RFC 52 LayerResultsGetShape support for OGR connection type.

- Fixed uninitialized variable with malloc used in osPointCluster() (#3236)

- Oracle driver: remove BLOB columns instead of changing them to null (#3228)

- Fixed ogc sld functions to return proper values (#2165)

- MAP EXTENT truncates GetFeature requests for data outside bounds (#1287)

- Added msStringSplitComplex function (#471)

- Mapserver WFS should send maxfeatures to the spatial database (#2605)

- WFS paging support (#2799)

- Fixed joins do not accept encrypted passwords (#2912)

- Fixed HTML legend image samples truncated (#2636)

- WMS GetFeatureInfo should respect the scale like GetMap does (#842)

- Filter encoding: simple filters using propertyislike not applied properly
  #2720, #2840

- Fix VBAR Chart production when using GD for rendering (#3482)


Version 5.6.0 (2009-12-04):
---------------------------

- WFS hits count is incorrect if the request contain 2 layers or more (#3244)

- Fixed a problem with layer plugin where copyVirtualTable didn't copy
  the LayerResultsGetShape function pointer (#3223)


Version 5.6.0-rc1 (2009-11-27):
-------------------------------

- Fixed a problem with shape-based queries against projected layers when 
  using a tolerance (#3211)

- Fixed long expression evaluation (#2123) 

- Added simplfy and topologyPreservingSimplify to MapScript (#2753)

- Fixed Oracle FastCGI memory leak (#3187)
 
- layer->project flag not being reset properly for drawquerylayer (#673 #2079)

- OGC SLD: support multi-polygons geometries for filters embedded in 
  an SLD (#3097)

- [WMC] embedded SLD in context does not work with namespace prefix (#3115)

- Support name aliases used in sld text symbolizer (#3114)

- decode html and unicode entities for polygon truetype symbol fills (#3203)

- Parse PropertyName parameter for wfs requests (#675)

- Fixed when saving a map, layer->transform isn't written properly in 
  all cases. (#3198)

- Fixed buffer overflow in oracle spatial driver with large sql data (#2694)

- Improve FastCGI include file finding logic (#3200)


Version 5.6.0-beta5 (2009-11-04):
---------------------------------

- Apply a minimum width on label outline (new outlines were too thin by default)

- Don't apply scalefactor to polygon outline widths (but apply the 
  resolutionfactor)

- Fix vector symbol size calculation (#2896)

- Applied code clean up patch for mapsearch.c. (#3189)

- Fixed labels centering when the label is longer than the line (#2867)

- Ensure Python MapScript building doesn't reorder the libraries, support the 
  'subprocess' module where available for setup.py, and default to using the 
  "super" swig invocation described in the Python MapScript README when 
  mapscript_wrap.c isn't available on the file system.  #2663 contains the 
  reordering issue.

- Fixed memory leak with shapefiles associated with one-pass query 
  implementation (#3188)

- Fix abs/fabs usage that prevented angle follow labels to be discarded if 
  they were too wrapped on themselves

- Allow CGI mapshape and imgshape variables to consume WKT strings (#3185)

- Added support for nautical miles unit (#3173)

- Fixed encoding metadata ignored by a few wcs/wfs 1.1 and sos requests (#3182)

- PHP/Mapscript: added "autofollow" and "repeatdistance" in labelObj (#3175)

- Added charset in content-type http header for wms/wfs/sos/wcs requests (#2583)

- Python/MapScript: improve compatibility for different swig versions (#3180)

- maprasterquery.c: a few fixes since beta4 (#3181, #3168).

- mapows.c: Generate WMS LatLongBoundingBox in WGS84 datum (#2578)


Version 5.6.0-beta4 (2009-10-18):
---------------------------------

- Allow processing of single shapefiles with no items (e.g. an empty dbf file) (#3147)

- Added resolution scaling for swf, svg, pdf and imagemap drivers (#3157)

- Correct cases where a valid WFS Layer might return errors if
  map extent does not overlap the layer extent (#3139)

- fix problem with 0-length line patterns in AGG

- Fixed problem of text/html WMS GetFeatureInfo which was returning HTML 
  image map output instead of the expected text/html template output.
  This was done by changing the imagemap outputformat to use the 
  "text/html; driver=imagemap" mime type (#3024)

- more resolution fixes for resolution scaling (symbolscale case) (#3157)

- Make sure layer extents are saved when a mapfile is written (#2969)

- Fixed CurvePolygons from oracle not drawn (#2772)

- Fixed raster queries (broken by RFC 52 changes) (#3166)

- Fixed coordinate projection problem in some cases with WMS GetFeatureInfo
  output in GML2 format (#2989)

- Added resolution scaling of some properties for GD driver (#3157)

- Modified GD functions API to be consistent with all others drivers (#3157)

- OGC Filter: strip all namespaces (not only ogc, gml) (#1350)

- Use decimal values for size and width in SVG output format (#2835)

- Correct invalid test when loading movies in an swf output (#2524)

- Return WMS GetCapabilities v1.3.0 by default as required by spec (#3169)

- Fixed mapObj.zoomScale() and mapobj.zoomRectangle() scaling problem in
  Mapscript (#3131)

- Few more fixes for high res output (#3157)

- Allow "DRIVER 'TEMPLATE'" or "DRIVER TEMPLATE" in output formats

- Correct sld generated from mapserver classes (#3133) 

- Correct libjpeg v7 compatibility issue in old jpeg interface code (#3167)

- Correct FEATURE_COUNT limits on WMS GetFeatureInfo raster queries (#3168)

- Correct SCALE_BUCKETS issue with 16bit raster scaling (#3174)


Version 5.6.0-beta3 (2009-10-07):
---------------------------------

- make RFC55 highres output be friendly with scaledependant rendering (#3157)

- avoid fractured and overlapping glyphs with angle follow (#2812)

- Fixed SDE layer seg fault (#3152)

- Fixed placement of labels using ANGLE AUTO which were not always positioned 
  correctly (#3160)

- Enable output of geometry in GML GetFeatureInfo if wms_geometries and
  wms_geom_type are specified (#2989)

- fix URN typo in mapwfs.c for urn:EPSG:geographicCRS:...

- don't apply scalefactor to label outlines (#3157)

- update namespaces and schema pointers (#2872)

- add RFC49 and RFC40 keywords to copy functions (#2865)

- minor fix to correct numberOfResults when maxfeatures is set in 
  mapfile (#2907)

- Fixed possible crash with WFS GetFeature when no projection is specified 
  at the map level (#3129)

- Fixed anchor point of vertically stacked bar graphs

- Fixed TEXT property that cannot be removed in the CLASS object. 
  PHP/Mapscript (#3063)

- Fixed use of minfeaturesize auto with curved labels (#3151)

- Fixed msRemoveHashTable function when 2 keys have the same hash (#3146)

- Fix raster thread deadlock condition on posix/linux (#3145)

- Do not route thread debug output through msDebug which requires locking.

- Fix WCS coverage metadata handling if size/resolution missing (#2683).

- Fix WCS crash with use of datasets that aren't physical files (#2901).

- Fix WCS failure with WCS 1.1 OGC URN (urn:ogc:def:crs:OGC::CRS84) (#3161).


Version 5.6.0-beta2 (2009-10-01):
---------------------------------

- Fixed a couple of issues with Oracle Spatial and single pass queries (#3069)

- Added layer.resultsGetShape() to PHP MapScript for use with queries (#3069)

- Fixed query maps under the new single pass query process (#3069)

- WFS Client seg fault (OGR layer not opened) (#3136)

- Reduce use of sqrt() calls when determining distances (#3134)

- support axis ordering for WFS 1.1 (#2899)

- const changes to avoid warnings with msLoadProjectionString()

- mapgd.c: removed unused drawVectorSymbolGD() function.

- Use http://www.mapserver.org/mapserver namespace URI in XML mapfile 
  schema (#3142)

- Fixed issue with PHP_REGEX_INC in mapscript/php3/Makefile.in (#3078)


Version 5.6.0-beta1 (2009-09-23):
---------------------------------

- WMS 1.3.0 expects a CRS parameter instead of SRS (#2979)

- Allow users to set wms getmap and getlegendgraphic image format list (#455)

- Fixed MapScript shapeObj->toWkt() segfaults (#2763)

- add vertical bar charts to dynamic charting (#3128)

- Get the intersection points and labels #3101

- Fixed shp2img not to cause a crash when the map couldn't be loaded

- Fix problem with overflowing shape->index for (most) query modes (#2850)

- Useful error message when PK is missing and data is subselect (#3124)

- Add WMS root Layer metadata support (#3121)

- Fixed build problem of PHP/Mapscript when php is compiled with gd as
  a shared extension (#3117)

- Improve safety of srcx/y checks in nearest neighbour raster resampler (#3120)

- Added support for 4d geometry types and oci bind variables for Oracle (#3107)

- Implement SECTION support for the WCS 1.1 GetCapabilities request (#3105)

- Fixed WCS processing when both crs and response_crs are specified (#3083)

- Fixed msFreeMap causing memory corruption in msFreeOutputFormat (#3113)

- Fix WMC XML output when Dimension is used (#3110)

- Enable LOAD_WHOLE_IMAGE processing option by default when rendering
  WMS client layer images (#3111). 

- add default values for CGI %key% substitutions (#3108)

- fix clipping of polygon shapes in line layers (#3059)

- RFC 51 implementation: XML Mapfiles Format (#2872)

- Fix output for valid WCS 1.1 XML (#3086)

- Avoid double free with postgresql joins. (#2936)

- Don't attempt to project layers given in pixel coordinates 
  (layer->transform != MS_TRUE) (#3096)

- Modify loading imagery from GDAL to load all bands at once to avoid multiple
  passes through pixel interleaved data (mapdrawgdal.c).  This is just an
  optimization - there should be no change in results or features.

- Modern GDALs clear the config when destroying driver manager.  Skip this
  call to avoid TLS leakage on cleanup (mapgdal.c).

- Fixed msMSSQL2008LayerGetShape to retrieve proper wkb geometries (#3082)

- Fixed the shape index for the inline layers (#3074)

- Fixed MINDISTANCE not considering label size on lines (#3050)

- Labeling enhancements: ability to repeat labels along a line/multiline (#3030)

- Modified STYLEITEM code to use the new way of rendering thick lines (#3025)

- Fixed template processor to respect layer order. (#2619)

- Add MS_DEBUGLEVEL and MS_ERRORFILE commandline switches for mapserv.c. 

- Exposed msMapOffsetExtent, msMapScaleExtent and msMapSetCenter methods 
  in PHP/Mapscript (#2460)

- Removed ZEND_DEBUG define in PHP/Mapscript (#2717)

- Fixed PHP/Mapscript to support PHP 5.3 (#3065, #3066)

- remove -O optimization flags to configure script if configured
  with --enable-debug

- Fixed performance bottleneck when computing a polygon center of gravity for
  label point computation. (#3053)

- make WFS numberOfFeatures match maxFeatures if passed (#2907)

- Add logging in layer visibility tests to help users find why layers 
  don't draw (#3054)

- include PNG libs first (#3046)

- merge graphics branch: RFC54 implementation, cairo rendering (png, svg, pdf),
  opengl rendering (non functional yet)

- Do pre-emptive test for map.extent/layer.extent interaction (#3043)

- Add centroid geomtransform (#2825)

- Test database connections before using them (#2932)

- Support non-numeric join criteria (#2006)

- Ensure there's enough room in the SQL to hold a long (#1284)

- Fix filter error in multi-clause filters (#2937)

- Fix agg freetype character lookup when no unicode charmap is present (#3018)

- Fix memory leak in SQL building (#2997)

- Fork AGG rendering library in our trunk

- Fixed a memory leak when unescaping quotes in	logical	expressions (#2938)

- Fixed template code for item, shpxy and extent tags to properly initialize
  tag arguments in cases where there are multiple tags in one chunk of
  template (#2990)

- Fix mapogcfilter.c not to cause syntax error if PROJ.4 is not compiled 
  in (#2987)

- Rework Python MapScript's setup.py to be more like Python's to fix
  a number of issues including (#2637) and to use mapserver-config 
  and ditch the old mapscriptvars approach

- Prevent from changing the connection type to MS_RASTER when 
  setConnectionType(MS_WMS) is used (#2908)

- Improve rounding logic for computing the src_xoff/yoff (#2976)

- Fix filename path processing for raster queries and WCS get coverage so 
  that non-filesystem filenames are not altered (#2901) 

- Improved security relative to untrusted directories and mapfiles (RFC 56)

- Fixed several security issues found in an audit of the CGI 
  application (#2939, #2941, #2942, #2943, #2944)

- setConnectionType(MS_WMS) doesn't work with mapscript (#2908)

- Perl Mapscript: improvement of imageObj wrapper (#2962)

- Improve control of output resolution (RFC 55, #2948)

- mapraster.c: use GDALOpenShared(), and CLOSE_CONNECTION=DEFERRED (#2815)

- AGG font outline method change (#1243)

- Change mapbit.c bitmask type from char to new 32bit ms_bitarray (#2930)

- Added resolution writing in image files (#2891)

- Try to save resolution values to GeoTIFF via GDAL (#2891)

- Refactor legend icon drawing (remove renderer specific versions)
  Add label styling or markers for annotation layer legend icons (#2917)

- Update EXTENT warning message (#2914)

- add support for SRSNAME parameter (#2899)

- add support for resultType (#2907)

- WFS 1.1.0 should use OWS Common 1.0.0 (#2925)

- clean up GEOS init and cleanup functions (#2929)

- add support for disabling SLD (#1395)

- fix to output gml:boundedBy again (#2907)

- fix warning for change in bitmask type (#2930)

- fix time advertising in WMS 1.3.0 (#2935)

- fix SOS blockSeparator output (#3014)

- fix MIME type support (#3020)


Version 5.4.0-beta3 (2009-3-5):
--------------------------------

- SLD: Correct crash with large class names (#2915)

- Added Java MapScript WIN64 support (#2250)

- Fixed a problem with long running processes, embedded scalebars/legends 
  and AGG. (#2887)

- Applied patch to deal with a couple of WCS issues (time ranges, #2487) 
  and PostGIS tileindex-based time filters (#1856)

- Adding -DUSE_GENERIC_MS_NINT to the WIN64 MapScript builds (#2250)

- Fixed C# the compiler options for MSVC 2008 (#2910)

- Fix build problem with mapogcsld.c when OWS services are not available (#473)

- Fix build on windows (maputil.c)

Version 5.4.0-beta2 (2009-2-25):
--------------------------------

- Fixed a problem where default shade symbols (solid fill, no size) were being
  scaled and not rendered as expected (related to #2896 I believe)

- Fixed a problem with offset polylines (AGG only) (#2868)

- Generate SLD version 1.1.0 (#473) 

- Tracking original geometry type so we can make better decisions on what
  positions to use with POSITION AUTO and annotation layers. (#2770)

- Setting up the same size units between the OGR auto-style and the 
  OGR label attribute binding option (#2900)

- Take better care of the extra items with the inline layers to 
  prevent from memory corruption (#2870)

- Fixed the compiler options for MSVC 2008 (#2898)

Version 5.4.0-beta1 (2009-2-18):
--------------------------------

- restored much of the pre-5.0 capabilities to update a mapfile via URL but in
  a more secure manner (RFC44)

- WMS 1.3.0 support added (#473)

- OWS GetCapabilities should skip layers with status == MS_DELETE (#2582)

- Set the default symbol size to 1 instead of 0 (#2896)

- fix WMS LegendURL to print sld_version for 1.3.0 Capabilities (#473)

- add GetSchemaExtension to WMS to support GetStyles in Capabilities XML (#473)

- move xlink declaration to root of WMS 1.3.0 DescribeLayerResponse

- Fixed a scalebar rounding problem causing to draw zero scalebar width (#2890)

- SLD: if it contains a Filter Encoding tag, try to always set the
  layer's FILTER element (#2889)

- Add support for rendering INLINE layers with layer attributes (items) (#2870)

- Fix mapserver crash when rendering a query map in HILITE mode
  and there are is no STYLE defined (#2803)

- Added projection support to [...ext] tags for template output.

- Removed the error generation when the OGR layer contains no fields (#2883)

- Added enhancements to mapogr.cpp for style annotations (#2879)

- Fixed memory leaks when using msUpdate*FromString methods. (#2857)

- Fixed the problem when removing the attribute binding in mapscript.

- SOS XML validity fixes (#2646)

- add WFS calls for schema resolution (#2646)

- add gml:id to om:Observation (#2646)

- fix some XML validity issues (#2646)

- Fixed endianness issues with wide character strings for ArcSDE (#2878).  
  Thanks Russell McOrmond

- Fixed WMS request with LAYERS parameter: may cause segmentation fault (#2871)

- fix when layer status = DEFAULT and passing list of layers (#2066)

- Fixed msAddLabel may cause access violation in certain conditions

- Changed base type of labelObj size, minsize and maxsize from int to double (#2766)

- add support for WMS Server title in LAYER object (#885)

- Fixed build problem using --with-gd=static and freetype (#2697)

- RFC49 implementation (#2865)

- Fixed Blobs not filtered in OracleSpatial Attribute/WFS queries (#2829)

- Fixed memory leak of map::setProjection in PHP/MapScript (#2861)

- Fixed "internal PHP GC memory leaks" in PHP/MapScript (#2767)

- Fixed bug with wms layer group hierarchy (#2810)

- Added updateFromString() methods for several objects in PHP/Mapscript (#2298)

- Added ms_newMapObjFromString mapObj constructor in PHP/Mapscript (#2399)

- Add support to compile mssql2008 when SDE or ORACLE is not compiled (#2851)

- Add support for creating debug builds for the plugins on Windows

- Correct half pixel error in WMS layer's BBOX request to remote WMS (#2843)

- Expose Map/Layer's Projection objects in PHP/MapScript (#2845)

- Added getUnits() methods to projectionObj in Mapscript (#2798)

- Improved Tag parsing in template code. (#2781)

- Added hashtable object and metadata methods for php-mapscript (#2773)

- mappostgis.c: Fix trailing spaces in fixed varchar fields (#2817)

- RFC48 implementation: GEOMTRANSFORM on styleObj (#2825)

- mapwms.c: cleanup warnings with recent gcc versions (#2822)

- mapogcsos.c: Cleanup warning and error messages

- mapagg.cpp: Fix center of rotation for truetype marker symbols 

- mapowscommon.c: use msLibXml2GenerateList to generate listed XML elements

- mapowscommon.c: output version string correctly (#2821)

- Added removeLayer function to mapObj in PHP/MapScript. (#762)

- Exposed PIXELS value via URL configuration

- Add Support for SLD TextSymbolizer HALO and ANGLE (#2806)

- IGNORE_MISSING_DATA: largely replaced by run-time CONFIG property,
  ON_MISSING_DATA, which supports three modes: FAIL, LOG, and IGNORE.
  (#2785) ms-rfc-47.txt

- mapstring.c: msStringTrim(*char str), front-and-back whitespace trimmer

- mappostgis.c: re-write to remove binary cursors and break up 
  logic into smaller parts, add support for maxfeatures

- mapogcfilter.c: increase array size (code was assigning to out
  of bounds subscript)

- MapScript: Added getBinding method to label and style object (#2670)

- mapowscommon.c: use strcasecmp to check for language value

- raster query fix for tileindex with relative paths (#2722)

- Fixed msOGRGetValues function to return default values if the object 
  type is not TEXT. (#2311)

- Fix for the access violation caused by msMSSQL2008LayerGetShape (#2795)

- Fixed msMSSQL2008LayerGetItems to retrieve the column names properly (#2791)

- Prevent from calling msMSSQL2008CloseConnection from msMSSQL2008LayerClose
  causing memory corruption issues (#2790) 

- new polygon label placement algorithm (#2793)

- stop drawing an artificial outline around polygons to ensure
  continuity - users needing this feature will have to explicitly
  add an outlinecolor of the same color as the fill color

- added formatoption QUANTIZE_NEW to force going through the pngquant 
  quantization algorithm instead of the GD one for imagemode RGB (the 
  GD one can be kind of buggy)

- fix some integer rounding errors in the agg line offseter (#2659)

- fix a bug with shapes with duplicate end points. was causing nans
  in the angle follow placement code (#2695)

- refactor msGetLabelSizeEx (now merged with msGetLabelSize) (#2390)

- native label size computation for AGG when using angle follow (#2357)

- memory leak in msInsertLayer, from Ned Harding (#2784)

- label size computation refactoring (#2390)

- don't draw label background if we're using angle follow. (#2726)

- legend keyimage resampling with agg (#2715)

- tileindexed rasters when DATA is manipulated via mapscript work (#2783)

- styleObj width now supports attribute binding

- RFC40 implementation: label text wrapping and alignment (#2383)

- baseline adjustment for multiline labels (#1449)

- Added support to access to the labelObj OUTLINEWIDTH property in 
  PHP/MapScript 

- PHP paste image should also work with AGG (#2682)

- Fixed bug when QUERYMAP hilite color is set and the shape's color in a
  layer is from a data source (#2769)

- Decoupled AUTO label placement from the positions enum in mapserver.h. Added 
  explicit case for POLYGON layers where CC is the default and then we try UC, 
  LC, CL and CR. (#2770)

- Changed base type of styleObj size and width from int to double (#2766)

- Correct allocation error in mapmssql2008.c

- Add possibility  to use a full resolution setting for svg output (#1706)

- Fixed GetFeature through tileindex bug: the tileindex of the shape found 
  wasn't set properly in the resultcache object. (#2359)

- Removed comma to correct WCS 1.1 Coverages formatting in payload directory. 
  (#2764)

- Correct bug when LABEL_NO_CLIP in combination with minfeaturesize (#2758) 

- Fix a label size computation for AGG bug when scalefactor is used (#2756)

- various SOS updates for CITE compliance (#2646)

- Added support for static linking with the lib gd in configure 
  script (#2696)

- Support OpenLayer's ol:opacity extension to OGC Web Map Context docs (#2746)

- Added MS_VERSION_NUM for use with #if statements in code based on 
  libmapserver (#2750)

- Fixed the configure script: failed to detect php5 on ubuntu. (#2365)

- Fixed a memory leak associated with not deleting the lexer buffer
  before parsing certain types of strings. (#2729)

- Added legend graphics for layer of type annotation 
  for the AGG and GD renderer (#1523)

- Masking the out-of-range characters to avoid the crash 
  in the AGG renderer (#2739)

- Accept WMS requests in which the optional SERVICE parameter is missing. 
  A new test was incorrectly added in 5.2.0 that resulted in the error 
  "Incomplete WFS request: SERVICE parameter missing" when the SERVICE 
  parameter was missing in WMS requests in which the SERVICE parameter is 
  optional (#2737)

- Support for the MapInfo style zoom layering option (#2738)

- Implement Equals and GetHashCode properly for the mapscript C# classes

- Expose msConnectLayer to the SWIG mapscript interface with a new
  layerObj.setConnectionType() method that should be used instead of
  setting the layerObj.connectiontype directly (#2735)

- SLD: when creating well known symbols on the fly the pen-up value
  used should be -99.

- SWF: Button names reflects the layer id and shape id (#2691)

- Support reading projection parameter for OGC filters (#2712)

- Several enhancements to STYLEITEM AUTO support for labels (#2708) and
  TTF symbols (#2721) in OGR layers 

- Expose special attributes OGR:LabelText, OGR:LAbelAngle, OGR:LabelSize
  and OGR:LabelColor to MapScript getShape() calls (#2719)

Version 5.2.0 (2008-07-16):
---------------------------

- mapfile.c: Fixed a bug that prevented using named symbols via URL 
  configuration. (#2700)

Version 5.2.0-rc1 (2008-07-09):
-------------------------------

- mapowscommon.c: fix support multiple namespaces (#2690)

- Fix OGC simple filters on SDE layers (#2685)

- wfs11 getcapabilities: correct memory corruption (#2686)

- Allow building against Curl 7.10.6 and older which lack CURLOPT_PROXYAUTH
  option required for *_proxy_auth_type metadata (#571)

- Avoid fatal error when creating new ShapeFileObj with MapScript (#2674)

- Fixed problem with WMS INIMAGE exceptions vs AGG output formats (#2438)

- mapshape.c: Fixed integer pointer math being applied to uchars (#2667)

- Fixed seg fault with saveImage() in PHP MapScript due to #2673 (#2677)

- Fixed configure error related to new fribidi2 pkg-config support (#2664)

- Fixed windows build problem (#2676)

- Fix raster query bounds problem (#2679)


Version 5.2.0-beta4 (2008-07-02):
---------------------------------

- Added support in configure script for pkg-config for fribidi2 (#2664)

- Added more debug/tuning output to mapserv and shp2img at debug level 2 (#2673)

- maptemplate.c: removed extra line feeds from mime header output. (#2672)

- mapresample.c: fix for bug 2540 when using raster resampling and AGG.

- mapsde.c: Check at compile time that we have SE_connection_test_server, 
  which appears to only be available for ArcSDE 9+ (#2665).
  
- mapshape.c: restore old behavior of tiled shapes relative to shapepath
  with new behavior for when shapepath is undefined (#2369)

- maputil.c: fix a bug for offset lines with agg, when the first segment 
  was horizontal (#2659)

- mapraster.c: fix for tiled rasters with relative shape paths defined, 
  from dfurhy (#2369)

- maptemplate.c: fixed a problem with extent tags with _esc extension not 
  working (#2666)

Version 5.2.0-beta3 (2008-06-26):
---------------------------------

- mapsde.c: processing option added to allow using fully qualified names
  for attributes (#2423).           

- mapsde.c: Test for an active connection before closing it (#2498).

- mapdraw.c: Fixed issue where path following labels were not being drawn 
  if FORCEd. (#2600)

- mapshape.c: Applied patch to make the location of tiled data relative to the
  tileindex directory if SHAPEPATH is not set. (#2369)

- maptemplate.c: Fixed issues in RFC 36 implementation that prevented mapscript 
  mapObj->processQueryTemplate() method from working.

- WMS/WFS: extend warning message (#1396)

- WFS: Respect units for the DWhitin parameter (#2297)

- WFS: correct OGC Contains filter  (#2306)

- WMS: set srsName correctly for GetFeatureInfo (#2502)

- SOS: detect invalid time strings (#2560)

- SOS: more srsName support (#2558)

- mapserv.c, maptemplate.c: fixed problem with arguments to msGenerateImages(). (#2655)

- WMS: produce warning if layer extent is null (#1396)

- WFS: project LatLongBoundingBox if required (#2579)

- SOS: generate error for some invalid filters (#2604)

- SLD: Use style's width parameter when generating sld (#1192)

Version 5.2.0-beta2 (2008-06-18):
---------------------------------

- mapogcsos.c: support invalid procedure in GetObservation (#2561)

- Fixed possible buffer overrun with Oracle Spatial driver (#2572)

- mapogcsos.c: support srsName in GetObservation (#2414)

- Filter Encoding: Modify DWithin definition (#2564)

- Added webObj legendformat and browseformat mapping in PHP MapScript (#2309)

- Removed static buffer size limit in msIO_*printf() functions (#2214)

- Fixed libiconv detection in configure for OSX 10.5 64 bit (#2396)

- mapstring.c: possible buffer overflow in msGetPath (#2649)

- maputil.c: Correct expression evaluation with text containing 
  apostrophes (#2641)

- mapwfs.c: Possibly generate an error message when applying filter 
  encoding (#2444)

- Added MS_LABEL_BINDING constants for SWIG MapScript (#2643)

- mapogcsos.c: fix POST support (#2379)

- maplibxml2.c: helper functions XML POST fix (#2379)

- mapwfs.c: fix segfault when srsName is not passed on BBOX Filter (#2644)

- mapwfs.c: do not return error for empty query results (#2444)

- Remove C++-style comments and most other warnings thrown by -pedantic (#2598)

- mapwfs.c/mapwfs11.c: set GML MIME type correctly

- mapogcsos.c: advertise supported SRS list via
  MAP.WEB.METADATA.sos_srs (#2414)

- mapwfs.c: set layer extent to map extent for default
  GetFeature requests with no spatial predicates (#1287)

Version 5.2.0-beta1 (2008-06-11):
---------------------------------

- WMS/WFS layers can now specify a proxy servert (#571)

- mapwmslayer.c: set QUERY_LAYERS correctly (#2001)

- mapwcs.c: handle PARAMETER values correctly (#2509)

- SOS: fix various memory leaks (#2412)

- mapwcs.c: advertise temporal support in GetCapabilities (#2487)

- Fixed flaw in findTag() in maptemplate.c that prevented multiple tags 
  on the same line being processed under certain conditions. (#2633)

- Return results even when extents are missing (#2420)

- Avoid displaying OGR connection strings in error messages (#2629)

- WCS: respect wcs_name metadata for GetCoverage and DescribeCoverage 
  requests (#2036)

- CGI: added -nh option to allow for the suppression of content headers from
  the command line (#2594)

- PostGIS: fix postgis idle-in-transaction problem (#2626)

- AGG: enable ellipse symbol rotation for POINT/ANNOTATION layers (#2617)

- RFC36: add more extensions to support templates (#2576)

- AGG: allow dashed hatch symbols (#2614)

- AGG: enable offset lines of type x -99 (#2588)

- AGG: use an agg specific label size calculation function where 
  possible (#2357)

- mapogcsld.c: fetch TextSymbolizer/Label/ogc:PropertyName correctly (#2611)

- Don't ignore .qix file when DATA reference includes .shp extension (#590)

- CGI able to alter layers with space and underscores (#2516)

- WFS Multipoint query with PostGIS bug fixed (#2443)

- Tiling API (RFC 43) mode=tile, tilemode=spheremerc, tile=x y zoom (#2581)

- Remove C++-style comments and most other warnings thrown by -pedantic (#2598)

- Fix PostGIS transaction behavior in fcgi situations (#2497, #2613)

- Improve performance for large shape files (#2282)

- encode WMS parameters correctly (#1296)

- Added alignment option within a scalebar (#2468)

- RFC-42 HTTP Cookie Forwarding (#2566)

- Fixed handling of encrypted connection strings in postgis driver (#2563)

- mapagg.cpp: AGG: add opacity at the style level (#1155)

- mapwms.c: add Cache-Control max-age HTTP header support (#2551)

- mapogcsos.c: support URI encoded procedures correctly (#2547)

- Added support for EMPTY template with WMS GetFeatureInfo (#546)

- Throw an exception if the WCS request does not overlap layer (#2503)

- Acquire TLOCK_PROJ for pj_transform() calls (#2533).

- Fixed problem with large imagemaps generating no output (#2526)

- mapwms.c: make version optional for GetCapabilities again (#2528)

- support URN scheme for components of observed property elements (#2522)

- Fixed gdImagePtr gdPImg memory leak in msSaveImageBufferGD() (#2525)

- mapogcsos.c: handle invalid POST requests (#2521)

- mapogcsos.c: handle ACCEPTVERSIONS parameter (#2515)

- mapwcs.c/mapwcs11.c: s/neighbour/neighbor/g (#2518)

- mapwms.c: relax FORMAT parameter restrictions for GetFeatureInfo (#2517)

- mapwcs.c: support COVERAGE lists for DescribeCoverage (#2508)

- mapwcs.c: fix lonLatEnvelope/@srsName (#2507)

- mapwcs.c: omit VendorSpecificCapabilities (#2506)

- mapwcs.c: test for either resx/resy OR width/height (#2505)

- mapwcs.c: make GetCoverage demand one of TIME or BBOX (#2504)

- mapwms.c: make GetLegendGraphic listen to TRANSPARENT in OUTPUTFORMAT (#2494)

- OWS: support updatesequence (#2384)

- mapwms.c: test VERSION after service=WMS (#2475)

- OWS: output Capabilities XML updateSequence if set (#2384)

- mapwcs.c: better handling of REQUEST parameter (#2490)

- mapwcs.c: point to correct exception schema (#2481)

- mapows.c: add version negotiation (#996)

- mapwfs.c: return default GML2 when invalid OUTPUTFORMAT passed (#2479)

- mapowscommon.c: add OWS Common style version negotiation (#996)

- mapwcs.c: better section parameter handling for CITE (#2485)

- mapwfs.c: point to the correct schema for exceptions (#2480)

- shp2img.c/shp2pdf.c: clean up usage text, check for invalid layers (#2066)

- completed implementation of RFC24 (#2442, #2032)

- mapwcs.c: require VERSION parameter for DescribeCoverage and 
  GetCoverage (#2473)

- mapwcs.c: change error token to MS_WCSERR instead of MS_WMSERR (#2474)

- mapwcs.c: set exception MIME type to application/vnd.ogc.se_xml 
  for 1.0.0 (#2470)

- mapwcs.c: Generate a decently formatted exception if an WCS XML POST request
  is received (#2476).

- mapowscommon.c: support OWS Common 1.1.0 as well (#2071)

- mapogcsos.c: support SOS 1.0.0 (#2246)

- Implement mapObj.setCenter, mapObj.offsetExtent, mapObj.scaleExtent, 
  rectObj.getCenter at the SWIG API (#2346)

- mapogcfilter.c: use USE_LIBXML2 in ifdefs (#2416)

- clean up naming conventions of Shapefile API (#599)

- use msComputeBounds() instead, since it's already in the codebase (#2087)

- set shapeObj bounds from WKT (#2087)

- fixed issue where path following labels sometimes used the supplied
setting for position. In all cases with ANGLE FOLLOW we want to force
position MS_CC regardless of what is set in the mapfile.

- enforce (-99 -99) to be the penup value for vector symbols (#1036)

- Support for labeling features (polygon & line) prior to clipping. This
results in stable label positions regardless of map extent. (#2447)

- Support for quantization and forced palette png output with RGBA images
  (#2436)

- SLD using a single BBOX filter should generate an SQL ststement for 
  oracle/postgis/ogr (#2450)

- Accurate Calculation of legend size for WMS LegendURL (#2435)

- Converted mapogr.cpp to use OGR C API instead of C++ classes to allow
  GDAL/OGR updates without having to recompile MapServer (#545, #697)

- add missing space on dashed polygon outlines with svg (#2429) 

- Restored behavior of MS 4.10 and made WMS STYLES parameter optional
  again in GetMap and GetFeatureInfo requests (#2427)

- Speed up forced palette rendering (#2422)

- WMS GetFeatureInfo: honour FEATURE_COUNT for any INFO_FORMAT and 
  apply the FEATURE_COUNT per layer instead of globally (#2423, #1686)

- enable soft outlines on truetype labels. This is triggered with a new 
  keyword OUTLINEWIDTH for the LABEL block (#2417) 

- fix clipping rectangle to take width as well as size into account (#2411)

- AGG: added and use a line and polygon adaptor to avoid copying shapeObj
  points to an agg path_storage. avoids a few mallocs and a few copies.

- fixed symbolsetObj not to set the SWIG immutable flag permanently
  don't expose refcount and the symbol attributes (Ticket #2407)

- fix for support of entity encoded text in angle follow text (#2403)

- AGG: initial support for native computation of label sizes (#2357)

- AGG: support text symbols specified by their character number (#2398)  

- AGG: fix angle orientation for various symbols

- allow scientific notation for attributes binded to an int (#2394)

- merge GD and AGG label cache drawing functions (#2390)

- Enable AGG rendering of bitmap font labels instead of falling back to
  GD (#2387)

- clean up treatment of encoding and wrap character

- Fix legend label placement for multiline labels (#2382)

- enforce WRAP parameter in legend label (#2382)

- AGG: pixel level simplification for line and polygon shapes (#2381)

- fixed blue/green color swapping for space delimited strings bound to an
  attribute. (bug 2378)

- don't remove points that are checked as being colinear (#2366)

- add initial(?) support for reading a pie chart's size from an 
  attribute (#2136)

- don't bail out in map parsing if the outputformat had to be modified
  (bug #2321)

- use a renderer agnostic legend icon drawing function which switches
  to the GD or AGG specific one depending on the outputformat (#2348)

- AGG: switch alpha buffer when drawing query layer

- Fixed legend icons not drawing when using maxscaledenom

- AGG: fix embedded scalebar rendering when using postlabelcache (#2327)

- AGG: allow for fast and aliased rendering of simple lines and polygons
  thick lines and patterns (i.e. dashes)aren't supported.
  this is triggered when the symbol is of TYPE SYMBOL *and* its ANTIALIAS
  is off (waiting to find a better solution to trigger this).

- AGG: the pixmap of pixmap symbols is now cached in an agg-compatible state
  the first time it is accessed. this avoids rereading and retransforming 
  it each time that symbol is used.

- AGG: the imageObj now stores in what state it's alpha channel is in. The 
  number ofmsAlphaGD2AGG/AGG2GD calls is now reduced, but most importantly
  each of these calls is usually just a check for this state and does 
  no computation.
  
- AGG: fixed a few artifacts in embedded legend rendering on rgba images.

- Fixed modulus operator in the parser (#2323)

- maprasterquery.c: Fix crash when queries on done on raster layers with 
  no styles (#2343)

- maprasterquery.c: Modify msRASTERLayerOpen() to create a defaulted raster
  layer info if there isn't one, to avoid the errors about open only being
  supported after a query.  Also wipe the raster layer info in case of 
  an empty result set, or failures of a query to reduce likelihood of 
  leaking the raster layer info. 

- Improve out of memory handling in mapdrawgdal.c, and mapgd.c. (#2351)

- Improve configuration logic for fastcgi (#2355).

- WMS: image/wbmp should be image/vnd.wap.wbmp (#2360)

- SOS: support maxfeatures for GetObservation requests (#2353)

- mapdraw.c,mapquery.c: Reset layer->project flag for each full layer drawing 
  or query so that need to reproject will be reconsidered (#673).

- PHP MapScript: fix for getStdoutBufferString() and getStdoutBufferBytes()
  functions on win32 (#2401)

- mapowscommon.c: fix namespace leak issues (#2375)

- mapogcsos.c: add SWE DataBlock support (#2248)

- mapogcsos.c: fix build warnings, namespace and schema pointers (#2248)

- mappdf.c: support output in fastcgi case via msIO_fwrite() (#2406)

- mapogcsos.c: Initial support for POST requests (#2379) and updated 
  msSOSDispatch() handling

- mapogr.cpp: Use pooling api to ensure per-thread sharing of connections 
  only (#2408)

- mapogcsos.c: change substituted variable from sensorid to procedure (#2409)

- maplibxml2.c: Initial implementation of libxml2 convenience functions

- configure: Modified so libxml2 support is requested for WCS and SOS, 
  and is indicated by USE_LIBXML2 definition.  Use @ALL_ENABLED@ in 
  DEFINEs and mapscriptvars generation.

- mapresample.c: Fixed support for multi-band data in RAW mode for bilinear
  and nearest neighbour resamplers (#2364).

- mapdraw.c: Improve error reporting if a raster layer requested in
  a query map (#1842).

- mapfile.c: add simple urn:ogc:def:crs:OGC::CRS84 support.


Version 5.0.0 (2007-09-17)
--------------------------

- AGG: Fix angle computation for truetype marker symbols on lines (#2316) 

- Fix support for bilinear resampling of raster data with AGG (#2303)


Version 5.0.0-rc2 (2007-09-10)
------------------------------

- Prevent seg fault in msWMSLoadGetMapParams when request is missing (#2299)

- Fixed calculation of scale in PHP MapScript mapObj.zoomScale() (#2300)

- Fixed conflict between runtime substitution validation and qstring 
  validation.

- Fixed agg configure logic (now should work with --with-agg alone) (#2295)

- Fixed interleaving of multi-band results for raster query (#2294).


Version 5.0.0-rc1 (2007-09-05)
------------------------------

- Fixed "MinFeatureSize AUTO" labeling for polygon layers, works for polygon
  annotation layers too (#2232)

- Fixed path following labels with short (2/3 character) strings (#2223)

- AGG fix a bug when rendering polygons with tiled pixmaps

- Added requirement to provide validation pattern for cgi-based attribute
  queries using the layer metadata key 'qstring_validation_pattern' (#2286)

- Fixed msDebug causing a crash with VS2005 (#2287)

- Added stronger checks on libpdf version in configure script (#2278)

- Added msGetVersionInt() to MapScript (ms_GetVersionInt() in PHP) (#2279)

- _isnan prototype for MSVC builds from <float.h> #2277

- AGG: Fix a bug when rendering brushed lines with vector or pixmap symbols
  (artifacts could appear on outline)
  
- AGG: Adjust symbol height when brushing a line with a vector symbol so that
  the line width isn't truncated

- Only include process.h on win32 (non-cygwin) systems, moved from 
  maptemplate.h to mapserver.h.  (#2276)


Version 5.0.0-beta6 (2007-08-29)
--------------------------------

- Fixed problem with outline of polygons rendered twice with OGR
  STYLEITEM AUTO and AGG output (#2271)

- Fixed problem compiling with only WMS/WFS client but none of the 
  WMS, WFS, WCS or SOS server options enabled (#2272)

- Fixed buffer overflow in handling of WMS SRS=AUTO:... (#1824)

- AGG: render thick lines and polygon outlines with round caps and joins
  by default

- Typo in mapfile writing (#2267)

- Fixed mapping of class->keyimage in PHP MapScript (#2268)

- Look for libagg under lib64 subdir as well in configure (#2265)

- AGG: revert previous optimizations. now use caching of the rendering object
  to avoid the re-creation of some structures each time a shape is drawn

- AGG: optimizations for faster rendering. we now do not initialize the font
  cache when no text is to be rendered

- AGG: fixed rendering of polygons with holes (#2264)

- AGG - raster layers: fix typo in mapresample.c that produced random
  background colors when using OFFSITE (#2263)
 
- AGG: Fix a bug when rendering tiled polygons with truetype, pixmap or
  vector symbols (usually only affected bright colors)
  
- Avoid passing null to msInsertHashTable in processLegendTemplate
  when layer.name or layer.group not specified (#2261)

- Fixed problems with fonts in PDF output (#2142)

- AGG: smooth font shadows


Version 5.0.0-beta5 (2007-08-22)
--------------------------------

- Fixed XSS vulnerabilities (#2256)

- Allow building with AGG from source when libaggfontfreetype is missing.
  configure --with-agg=DIR now automatically tries to build 
  agg_font_freetype.o from source if libaggfontfreetype is missing (#2215)

- Fixed possible buffer overflow in template processing (#2252)

- fix blending of transparent layers with RGBA images

- AGG: speed up rendering of pixmap marker symbols

- Implement OGR thread-safety via use of an OGR lock (#1977).

- Fixed compile warnings (#2226)

- Fixed mappdf.c compile warnings, PDF support was probably unusable
  before that fix (#2251)

- Adding -DUSE_GENERIC_MS_NINT to the WIN64 builds (#2250)

- Adding msSaveImageBuffer and use that function from the mapscript
  library instead of the renderer specific functions. (#2241)

- Split each format into it's own <formats> element in WCS describe
  coverage results (#2244).

- Support to run the mapscript c# examples on x64 platform (#2240)

- Fixed problem introduced in 5.0.0-beta4: all HTML legend icons were
  empty white images (#2243)

- Fixed WMS Client to always send STYLES parameter with WMS GetMap
  requests (#2242)

- Fixed support for label encoding in SVG output (#2239)

- Added support for label encoding in legend (#2239)

- Fixed PHP MapScript layer->queryByAttributes() to not accept empty or
  null qitem arg (#480)

- AGG: fixed incorrect rendering of pixmaps on MSB architectures (#2235)

- Added layer.getFeature() in PHP MapScript with optional tileindex arg, 
  and deprecated layer.getShape() to match what we had in SWIG (#900)

- Added class.getTextString() and deprecated/renamed class.getExpression()
  and layer.getFilter() to class.getExpressionString() and 
  layer.getFilterString() to match what we have in SWIG MapScript (#1892)


Version 5.0.0-beta4 (2007-08-15)
--------------------------------

- Updated msImageCreateAGG to only allow RGB or RGBA pixel models (#2231)

- Fixed problem with symbol.setImagePath() when file doesn't exist (#1962)

- Python MapScript failures (#2230)

- msInsertLayer should not free the incoming layer anymore (#2229)

- Include only parsed in the first mapfile (#2021)

- Incorrect lookup of symbol in symbolset (#2227)

- Mapfile includes and MapScript (#2089)

- Fixed alignment of GetLegendGraphic output when mapfile contains no 
  legend object (#966)

- Fixed seg. fault when generaing HTML legend for raster layers with no
  classes (#2228). The same issue was also causing several Chameleon apps
  using HTML legend to seg fault (#2218)

- Do not use case sensitive searches in string2list, which is used
  for msWhichItems (#2067)

- Ensure that we can write AGG images with Python MapScript's write() method

- Support unicode attributes for ArcSDE 9.2 and above (#2225)

- GD: Truetype line symbolization should follow line orientation only
  if GAP is <=0

- AGG: Added truetype symbolization for lines and polygons

- AGG: Draw an outline of size 1 of the fill color around polygons if an
  outlinecolor isn't specified (avoids faint outline) 

- Added summary of options at end of configure output (#1966)

- Updated configure script to detect and require GEOS 2.2.2+ (#1896)

- Renamed --enable-coverage configure option to --enable-gcov to avoid
  confusion with WCS or Arc/Info coverages (#2217)

- Fixed --enable-gcov (formerly --enable-coverage) option to work with 
  php_mapscript.so (#2216)

- check for OGR support in if SLD is used (#1998) 

- msWMSLoadGetMapParams: fixed handling of required parameters (#1088)

- if any of srs, bbox, format, width, height are NOT found, throw exception

- if styles AND sld are NOT found, throw an exception

- NOTE: this may cause issues with some existing clients who do not pass
  required parameters


Version 5.0.0-beta3 (2007-08-08)
--------------------------------

Known issues: 

- This beta contains significant improvements and fixes on the AGG 
  rendering front. However some build issues remain on some platforms.
  Please see ticket #2215 if building with AGG support doesn't work with 
  the default configure script: http://trac.osgeo.org/mapserver/ticket/2215

Bug fixes:

- mapagg.cpp rewrite - the AGG renderer should now support all the GD features

- Use AGG when requested for drawing the legend

- Fixed problems with very large HTML legends producing no output (#1946)

- Use OGR-specific destructors for objects that have them rather than
  'delete' (#697)

- Include style-related info in HTML legend icon filenames to solve issues with
  caching of icons when the class or style params are changed (#745)

- Fixed issues with wms_layer_group metadata in WMS GetCapabilities (#2122)

- Use msSaveImageBufferAGG for AGG formats in getBytes (#2205).

- Make sure to emit $(AGG) to mapscriptvars because of conditional inclusion
  of struct members to imageObj. (#2205)

- Make imageextra field in imageObj not conditional (not #ifdef'ed) (#2205)

- AGG/PNG and AGG/JPEG are the only valid agg drivers.
  Imagetypes aggpng24 and aggjpeg can be used to refer to the
  default output formats. (#2195) 

- Fix memory leak with labepath object (#2199)

- Fix memory leak msImageTruetypePolyline (#2200)

- SWF: Fix incorrect symbol assignments (#2198)

- Fixed memory leaks in processing of WFS requests (#2077)

- Avoid use of uninitialised memory in msCopySymbol() (#2194)


Version 5.0.0-beta2 (2007-08-01)
--------------------------------

- Oracle Spatial: Fixed some issues related with the maporaclespatial.c 
  source code: warnings with calls in gcc 4.x versions (#1845), gtype 
  translation error, generating memory problem (#2056), problems with items
  allocation (#1961 and #1736), and some memory-leaks errors (#1662). 

- AGG: Fixed a significant number of rendering issues including conflicts with
  OPACITY ALPHA and ANTIALIAS TRUE settings w/regards to polygon fills. Fixed
  ellipse and vector markers. Fixed AGG/GD alpha channel conflicts by writing
  conversion to/from functions. (#2191-partial, #2173, #2177)

- SOS: Turn layer off if eventTime is not in the sos_offering_timeextent
  (#2154)

- WFS: Correct bugs related to query by featureid support (#2102)

- WMS: Add svg as a supported format for GetMap request (#1347)

- WMS: Correct WMS time overriding Filter parameter (#1261)

- Fix problem with LUT scaling ranges with explicit value for 255 (#2167).

- WCS: Fixed resampling/reprojecting for tileindex datasets (#2180)

- Fixed formatting of configure --help (#2182)
 
- Fixed AGG configure option to use 'test -f' instead of 'test -e' which
  doesn't work on Solaris (#2183)

- Fixed mapwms.c to support selecting AGG/ outputformats via FORMAT=.

- Removed unused styleObj.isachild member (#2169)


Version 5.0.0-beta1 (2007-07-25)
--------------------------------

New features in 5.0:

- MS RFC 19: Added Style and Label attribute binding

- MS RFC 21: Raster Color Correction via color lookup table

- MS RFC 27: Added label priority

- MS RFC 29: Added dynamic charting (pie and bar charts)

- MS RFC 31: New mechanism to load/set objects via URL using mapfile syntax

- MS RFC 32: Added support for map rendering using the AGG library for 
  better output quality


Long time issues resolved in 5.0:

- MS RFC 17: Use dynamic allocation for symbols, layers, classes and styles
  (got rid of the static limit on the number of instances of each in a map)

- MS RFC 24: Improved memory management and garbage collection for MapScript

- MS RFC 26: Terminology cleanup (layer transparency renamed to opacity,
  scale becomes scaledenom, symbol style becomes symbol pattern)

- MS RFC 28: Enhanced the debug/logging mechanism to facilitate 
  troubleshooting and tuning applications. Added support for multiple 
  debug levels and more control on output location.


Other fixes/enhancements in this beta:

- Upgrade Filter encoding to use geos and support all missing operators (#2105)

- Use of static color Palette support for gd output (#2096)

- MapServer's main header file map.h has been renamed mapserver.h (#1437)

- A mapserver-config script has been created

- Single and double quotes escaping in string expressions used by FILTER.
  (Resolves tickets #2123 and #2141)

- SLD: Support of Graphic Stroke for a Linesymbolizer (#2139)

- GD : draw symbols along a line using pixmap symbols (#2121)

- SVG : Polygons should not be filled if color is not given (#2055)

- WMS : fixed request with a BBOX and and SLD containing Filter 
  encoding (2079)

- SWF : use highlight color from querymap (2074)

- Support for embedding manifests as resources for the VS2005 builds.
  (ticket #2048)
 
- Changed OGRLayerGetAutoStyle not to pass NULL pointer to GetRGBFromString 
  causing access violation (bug 1950).
 
- Fix SDE returning the row_id_column multiple times (bug 2040).

- Fix text outline bug. (bug 2027)

- Improve error reporting when OWS services are requested but the support
  is not compiled in.  (bug 2025)

- Fix support for OFFSITE for simple greyscale rasters (bug 2024). 
 
- [SLD] : Error on last class in raster class names based on the ColorMapEntry
          (bug 1844)

- [Filter Encoding] : Check if Literal value in Filter is empty (bug 1995)

- [SLD] : Else filters are now generated at the end of classes (bug 1925)  

- Enabled setting of a layer tileindex (e.g. map_layername_tileindex) via the 
  CGI program. (bug 1992)

- Added feature to the CGI to check runtime substitutions against patterns
  defined in layer metadata. (bug 1918)

- Exposed label point computation to mapscript (bug 1979)

- [SLD]: use the url as symbol name for external symbols (bug 1985)

- [SLD] : support of mixing  static text with column names (bug 1857) 

- maperror.c: fix for wrapping long in image errors, thanks to Chris 
  Schmidt (bug 1963) 

- maperror.c: fix closing of stderr/stdout after writing error msg (bug 1970)

- Preliminary implementation of RFC 21 (Raster Color Correction). 

- [SLD] : when reading an SLD, sequence of classes was reversed (Bug 1925)

- Fixed a bug with SDE capability requests where we were double 
  freeing because sde->row_id_column wasn't set to NULL in msSDELayerOpen

- [OGC:SOS] : Fixed bugs related to metadata and xml output
  (1731, 1739, 1740, 1741).  Fixed bug with large xml output (bug 1938)

- fixed performance problem in raster reprojection (bug 1944)

- added msOWSGetLanguage function in mapows.c/h (bug 1955)

- added mapowscommon.c/mapowscommon.h and updated mapogcsos.c to use 
  mapowscommon.c functions (bug 1954)

- added more Perl mapscript examples in mapscript/perl/examples/, most
  of which exemplify recently added GEOS functionality

- php_mapscript.c: Fixed setRotation() method to check for MS_SUCCESS, not 
  MS_TRUE (bug 1968)

- mapobject.c: Fixed msMapSetExtent() to avoid trying to calculate the 
  scale if the map size hasn't been set yet (bug 1968) 

- mapobject.c: ensure msMapComputeGeotransform() returns MS_FAILURE, not 
  MS_FALSE (bug 1968)

- mapdraw.c: Actually report that we aren't configure with wms client
  support if that is why we can't draw a layer.

- mapows.c: fixed XML error (bug 2070)

- mapwms.c: Fixed text/plain output duplicate (bug 1379)

- mapwms.c: Attribution element output in GetCapabilities only 1.0.7 and 
  higher (bug 2080)

- mapwms.c: UserDefinedSymbolization element output in GetCapabilities 
  only 1.0.7 and higher (bug 2081)

- mapwms.c: GetLegendGraphic and GetStyles only appear in 1.1.1 and
  higher responses (bug 1826)

- mapwcs.c:
  - msWCSDescribeCoverage: throw Exception if Coverage doesn't exist (bug 649)
  - msWCSException: updated as per WCS 1.0 Appendix A.6

- mapogcsos.c: Added ability to output gml:id via MAP/LAYER/METADATA
  ows/sos/gml_featureid (bug 1754)

- mapcontext.c: Added ogc namespace (#2002)

- Note that starting with this release the source code is now managed 
  in Subversion (SVN) instead of CVS and we have migrated from bugzilla
  to Trac for bug tracking.


Version 4.10.0 (2006-10-04)
---------------------------

- No source code changes since 4.10.0-rc1

Known issues in 4.10.0:

- PHP5 not detected properly on Mandriva Linux (bug 1923)

- Mapfile INCLUDE does not work with relative paths on Windows (bug 1880)

- Curved labels don't work with multibyte character encodings (bug 1921)

- Quotes in DATA or CONNECTION strings produce parsing errors (bug 1549)



Version 4.10.0-RC1 (2006-09-27)
-------------------------------

- SLD: quantity values for raster sld can be float values instead of just 
   being integer

- Hiding labelitemindex, labelsizeitemindex, labelangleitemindex
  from the SWIG interface (bug 1906)

- Fixed computation of geotransform to match BBOX (to edges of image) not
  map.extent (to center of edge pixels).  (bug 1916)

- mapraster.c: Use msResampleGDALToMap() for "upside down" images. (bug 1904)


Version 4.10.0-beta3 (2006-09-06)
---------------------------------

- Web Map Context use format metadata when formatlist not available. (bug 1723)

- Web Map Context boolean values true/false now interpreted. (bug 1692)

- Added support for MULTIPOLYGON, MULTILINESTRING, and MULTIPOINT in 
  msShapeFromWKT() when going through OGR (i.e. GEOS disabled) (bug 1891)

- Fixed MapScript getExpressionString() that was failing on expressions
  longer that 256 chars (SWIG) and 512 chars (PHP). (bug 1428)

- WMSSLD: use Title of Rule if Name not present (bug 1889)

- Fixed syntax error (for visual c++) in mapimagemap.c. 

- Fixed mapgeos.c problems with multipoint and multilinestring WKT (bug 1897).

- Implemented translation via OGR to WKT for multipoint, multiline and
  multipolygon (bug 1618)


Version 4.10.0-beta2 (2006-08-28)
---------------------------------

- Applied patch supplied by Vilson Farias for extra commas with imagemap
  output (bug 760).

- Fixed possible heap overflow with oversized POST requests (bug 1885)

- Set ./lib and ./include properly for MING support (bug 1866)

- More robust library checking on OSX (bug 1867)

- Removed mpatrol support (use valgrind instead for something 
  similar and less intrusive).  (bug 1883)

- Added mapserver compilation flags to the SWIG c# command line (bug 1881)

- Fix OSX shared library options for PHP (bug 1877).

- Added setSymbolByName to styleObj for the SWIG mapscript in order to 
  set both the symbol and the symbolname members (bug 1835)

- Generate ogc filters now outputs the ocg name space (bug 1863)

- Don't return a WCS ref in WMS DescribeLayer responses when layer type is
  CONNECTIONTYPE WMS (cascaded WMS layers not supported for WCS) (bug 1874)

- Correct partly the problem of translating regex to ogc:Literal (bug 1644)

- schemas.opengeospatial.net has been shutdown, use schemas.opengis.net 
  instead as the default schema repository for OGC services (bug 1873)

- MIGRATION_GUIDE.TXT has been created to document backwards incompatible
  changes between 4.8 and 4.10

- Modify mapgd.c to use MS_NINT_GENERIC to avoid rounding issues. (bug 1716)

- added --disable-fast-nint configure directive (bug 1716)

- Fixed php_mapscript Windows build that was broken in beta1 (bug 1872)

- Supported <propertyname> tag in SLD label (Bug 1857)

- Use the label element in the ColorMapEntry for the raster symbolizer
  (Bug 1844)

- Adding Geos functions to php mapscript (bug 1327)

- Added a type cast to msio.i so as to eliminate the warning with the
  SWIG unix/osx builds

- Fixed csharp/Makefile.in for supporting the OSX builds and creating 
  the platform dependent mapscript_csharp.dll.config file.

- Fixed error in detection of libpdf.sl in configure.in (bug 1868).


Version 4.10.0-beta1 (2006-08-17)
---------------------------------

- Marking the following SWIG object members immutable (bug 1803)
  layerObj.metadata, classObj.label, classObj.metadata,
  fontSetObj.fonts, legendObj.label, mapObj.symbolset,
  mapObj.fontset, mapObj.labelcache, mapObj.reference,
  mapObj.scalebar, mapObj.legend, mapObj.querymap
  mapObj.web, mapObj.configoptions, webObj.metadata,
  imageObj.format, classObj.layer, legendObj.map,
  webObj.map, referenceMapObj.map
  labelPathObj was made completely hidden (according to Steve's suggestion)

- Fixed problem with PHP MapScript's saveWebImage() filename collisions
  when mapscript was loaded in php.ini with PHP as an Apache DSO (bug 1322)

- Produce warning in WFS GetFeature output if ???_featureid is specified
  but corresponding item is not found in layer (bug 1781). Also produce
  a warning in GetCapabilities if ???_featureid not set (bug 1782)

- Removed the default preallocation of 4 values causing memory leaks.
  (related to bug 1801) Added initValues to achieve the similar 
  functionality if needed.

- Fixed error in msAddImageSymbol() where a symbol's imagepath was not
  set (bug 1832).

- Added INCLUDE capability in mapfile parser (bug 279)

- Revert changes to mapzoom.i that swapped miny and maxy (Bug 1817).

- MapScript (swig) creation of an outputFormatObj will now set the inmapfile 
  flag so that it gets written out to saved maps by default (Bug 1816).

- Converted GEOS support to use the GEOS C-API (versiopn 2.2.2 and higher).
  Wrapped remaining relevant GEOS functionality and exposed via SWIG-based
  MapScript.

- If a layer has wms_timedefault metadata, make sure it is applied even
  if there is no TIME= item in the url.  (Bug 1810)

- Support for GEOS/ICONV/XML2 use flags in Java Makefile.in (related to 
  bug 1801)

- Missing GEOS support caused heap corruption using shapeObj C# on linux 
  (Bug 1801)

- Fix time filter propagation for raster layers to their tileindex layers.
  New code in maprasterquery.c (bug 1809)

- Added logic to collect LD_SHARED even if PHP not requested in configure.

- Fix problems with msio/rfc16 stuff on windows.  Don't depend on comparing
  function pointers or "stdio" handles.  (mapio.c, mapio.h, msio.i)

- Support WMC Min/Max scale in write mode (bug 1581)

- Fixed leak of shapefile handles (shp/shx/dbf) on tiled layers (bug 1802)

- Added webObj constructor and destructor to swig interface with 
  calls to initWeb and freeWeb (bug 1798).

- mapows.c: ensure msOWSDispatch() is always available even if there are
  no services to dispatch.  This makes mapscript binding easier.

- FLTAddToLayerResultCache wasn't properly closing the layer after it 
  was done with it.

- Added ability to encrypt tokens (passwords, etc.) in database connection
  strings (MS-RFC-18, bug 1792)

- Fixed zoomRectangle in mapscript: miny and maxy were swapped, making it
  impossible to zoom by rect; also the error message was referring to the
  wrong rect. There were no open issues on bugzilla. Reverted because of 1817.

- Implementation of RFC 16 mapio services (bug 1788).

- Use lp->layerinfo for OGR connections (instead of ogrlayerinfo) (bug 331)

- Support treating POLYGONZ as MS_SHAPE_POLYGON.  (bug 1784)

- Complete support for international languages in Java Mapscript
  (bug 1753)

- Output feature id as @fid instead of @gml:id in WFS 1.0.0 / GML 2.1.2
  GetFeature requests (bug 1759)

- Allow use of wms/ows_include_items and wms/ows_exclude_items to control
  which items to output in text/plain GetFeatureInfo. Making the behavior
  of this INFO_FORMAT consistent with the new behavior of GML GetFeatureInfo
  output introduced in v4.8. (bug 1761)
  IMPORTANT NOTE: With this change if the *_include_items metadata
  is not specified for a given layer then no items are output for that layer
  (previous behavior was to always all items by default in text/plain)

- Make sure mappostgis.c closes MYCURSOR in layer close function so that
  CLOSE_CONNECTION=DEFER works properly.  (bug 1757)

- Support large (>2GB) raster files relative to SHAPEPATH. (bug 1748)

- Set User-Agent in HTTP headers of client WMS/WFS connections (bug 1749)

- Detection of os-dependent Java headers for Java mapscript (bug 1209)

- Preventing to take ownership of the memory when constructing objects
  with parent objects using C# mapscript (causing nullreference exception, Bug 1743)

- SWF: Adding format option to turn off loading  movies automatically (Bug 1696)

- Fixed FP exception in mapgd.c when pixmap symbol 'sizey' not set (bug 1735)

- Added config file for mapping the library file so the DllImport 
  is looking for to its unix equivalent (Bug 1596) Thanks to Scott Ellington

- Added /csharp/Makefile.in for supporting the creation of Makefile 
  during configuration with MONO/Linux (fix for bug 1595 and 1597)

- Added C# typemaps for char** and outputFormatObj**

- Support for dispatching multiple error messages to the MapScript interface (bug 1704).

- Fix inter-tile "cracking" problem (Bug 1715).

- OGC FILTER: Correct bug when generating an sql expression containing an escape 
  character.

- Allow a user to set a PROCESSING directive for an SDE layer to specify
  using the attributes or spatial index first.  (bug 1708).

- Cheap and easy way of fudging the boundary extents for msSDEWhichShapes 
  in the case where the rectangle is really a point (bug 1699).

- Implement QUANTIZE options for GD/PNG driver (Bug 1690, Bug 1701).

- WMS: Publish the GetStyles operation in the capabilities document.

- PHP_MAPSCRIPT: Add antialias parameter in the style object (Bug 1685)

- WFS: Add the possibility to set wfs_maxfeatures to 0 (Bug 1678)

- SLD: set the default color on the style when using default settings
  in PointSymbolizer. (bug 1681)

- Incorporate range coloring support for rasters (bug 1673)

- Fixed mapthread.c looking for the unix compiler symbol rather than just 
  testing whether or not _WIN32 is defined for the usage of posix threads
  because unix is not defined on compilers like GCC 4.0.1 for OS X.

- Fixed the fuzzy brush support so that the transition between 1 pixel aa lines 
  and brushes is less obvious. The old code would not allow for a 3x3 fuzzy 
  brush to be built. (bug 1659)

- Added missing mapscript function msConnPoolCloseUnreferenced() (bug 1661)
  We need to make conn. pooling handling transparent to mapscript users
  so that they do not have to call this function once in a while, for instance
  by creating an evictor thread.

- Added calls to msSetup/msCleanup() at MapScript load/unload time (bug 1665)

- Reorganized nmake.opt to be more focused on functionality groups rather 
  than the propensity of a section to be edited.  Default values are now 
  all set to be pointed at the MapServer Build Kit, which can be obtained 
  at http://hobu.stat.iastate.edu/mapserver/

- configure.in/Makefile.in: Use PROJ_LIBS instead of PROJ_LIB.  PROJ_LIB
  is sometimes defined in the environment, but points to $prefix/share/proj
  not the proj link libraries.  

- Update Web Map Context to 1.1.0, add the dimension support. (bug 1581)

- Support SLD body in context document. (bug 887)

- When generating an ogc filter for class regex expressions, use
  the backslah as the default escape character (Bug 1637)

- Add connectiontype initialization logic when the layer's virtual
  table is initialized (Bug 1615) 

- Added modulus operator to mapparser.y.

- Added new support for [item...] tag in CGI-based templates (bug 1636)

- Reverted behavior to pre-1.61:
  do not allow for use of the FILTERITEM attribute (bug 1629)

- Treat classindex as an int instead of a char in resultCacheMemberObj to
  prevent problems with more than 128 classes (bug 1633)

- WMS : SLD / stretch images when using FE (Bug 1627)

- Add gml:lineStringMember in GML2 MultiLineString geometry (bug 1569).

- PHP : add shape->sontainsshape that uses geos lib (Bug 1623).

- Move gBYTE_ORDER inside the pg layerinfo structure to allow for differently
  byte ordered connections (bug 1587).

- Fix the memory allocation bug in sdeShapeCopy (Bug 1606)

- Fixed OGR WKT support (Bug 1614). 

- Added shapeObj::toWkt() and ms_shapeObjFromWkt() to PHP MapScript (bug 1466)

- Finished implementation of OGR Shape2WKT function (Bug 1614).

- Detect/add -DHAVE_VSNPRINTF in configure script and prevent systematic
  buffer overflow in imagemap code when vsnprintf() not available (bug 1613)

- Default layer->project to MS_TRUE even if no projection is set, to allow
  geotransforms (nonsquare pixels, etc) to be applied (bug 1645).

- Force stdin into binary mode on win32 when reading post bodies. (bug 1768)


Version 4.8.0-rc2 (2006-01-09)
------------------------------

- Commit fix for GD on win32 when different heaps are in use. (Bug 1513) 

- Correct bound reprojection issue with ogc filer (Bug 1600)

- Correct mapscript windows build problem when flag USE_WMS_SVR was
  not set (Bug 1529)   

- Fix up allocation of the SDE ROW_ID columns and how the functions that
  call it were using it. (bug 1605)

- Fixed crash with 3D polygons in Oracle Spatial (bug 1593)


Version 4.8.0-rc1 (2005-12-22)
------------------------------

- Fixed shape projection to recompute shape bounds. (Bug 1586)

- Fixed segfault when copying/removing styles via MapScript. (Bug 1565)

- Fixed segfault when doing attribute queries on layers with a FILTER already
  set but with no FILTERITEM.


Version 4.8.0-beta3 (2005-12-16)
--------------------------------

- Initialize properly variable in php mapscript (Bug  1584)

- New support for pseudo anti-aliased fat lines using brushes with variable
  transparency. 

- Arbitrary rotation support for vector symbols courtesy of Map Media.

- Support for user-defined mime-types for CGI-based browse and legend 
  templates (bug 1518).

- mapraster.c: Allow mapresample.c code to be called even if projections
  are not set on the map or layer object.  This is no longer a requirement.
  (Bug 1562)

- Fix problem with WMS 1.1.1 OGC test problem with get capabilities dtd 
  (Bug 1576) 

- PDF : adding dash line support (Bug 492)

- Fixed configure/build problem (empty include dir) when iconv.h is not
  found (bug 1419)

- PDF :  segfault on annotation layer when no style is set (Bug 1559)

- PostGIS layer test cases and fix for broken views and sub-selects (bug 1443).

- SDE: Removed (commented out) support for SDE rasters at this time.  As far 
  as I know, I'm the only one to ever get it to work, it hasn't kept up with 
  the connection pooling stuff we did, and its utility is quite limited in 
  comparison to regular gdal-based raster support (projections, 
  resampling, etc) (HB - bug 1560).

- SDE: Put msSDELayerGetRowIDColumn at the top of mapsde.c so things 
  would compile correctly.  This function is not included (or necessary) 
  in the rest of the MS RFC 3 layer virtualization at this time.
  
- WFS : TYPENAME is manadatory for GetFeature request (Bug 1554).

- SLD : error parsing font parameters with the keyword "normal" (Bug 1552)

- mapgraticule.c: Use MIN/MAXINTERVAL value when we define grid position and 
  interval (bug 1530)

- mapdrawgdal.c: Fix bug with nodata values not in the color table when 
  rendering some raster layers (bug 1541). 

- mapogcsld.c : If a RULE name is not given, set the class name to "Unknown" 
  (Bug 1451)

Version 4.8.0-beta2 (2005-11-23)
--------------------------------

- Use dynamic allocation for ellipse symbol's STYLE array, avoiding the
  static limitation on the STYLE argument values. (bug 1539)

- Fix bug in mapproject.c when splitting over the horizon lines.  

- Fix Tcl mapscript's getBytes method (bug 1533).

- Use mapscript.i in-place when building Ruby mapscript, copying not necessary
  (bug 1528).

- Expose maximum lengths of layer, class, and style arrays in mapscript (bug
  1522).

- correct msGetVersion to indicate if mapserver was build with MYGIS support.

- Fixed hang in msProjectRect() for very small rectangles due to round off
  problems (bug 1526).


Version 4.8.0-beta1 (2005-11-04)
--------------------------------

- Bug 1509: Fixed bounding box calculation in mapresample.c. The bottom right
  corner was being missed in the calculation.

- MS RFC 2: added OGR based shape<->WKT implementation.

- mapgdal.c: fixed some mutex lock release issues on error conditions.

- MS RFC 8: External plugin layer providers (bug 1477)

- SLD : syntax error when auto generating external symbols (Bug 1508).

- MS RFC 3: Layer vtable architecture (bug 1477)

- wms time : correct a problem when handling wms times with tile index rasters
  (bug 1506).

- WMS TIME : Add support for multiple interval extents (Bug  1498)

- Removed deprecated --with-php-regex-dir switch (bug 1468)

- support wms_attribution element for LAYER's (Bug 1502)

- Correct php/mapscript bug : initialization of scale happens when
  preparequery is called (Bug 1334).

- msProjectShape() will now project the lines it can, but completely
  delete lines that cannot be projected properly and "NULL" the shape if
  there are no lines left. (Bug 411)

- Expose msLayerWhichShapes and msLayerNextShape in MapScript. (bug 1481) 

- Added support to MapScript to change images in a previously defined
  symbol. (bug 1471)

- mapogcfiler.c : bug 1490. Crash when size of sld filters was huge.

- Fixed --enable-point-z-m fix in configure.in (== -> =) (bug 1485).

- Extra scalebar layer creation is prevented with a typo fix in mapscale.c. 
  Good catch, Tamas (bug 1480).
  
- mapwmslayer.c : use transparency set at the layer level on wms client 
                  layers  (Bug 1458) 

- mapresample.c: added BILINEAR/AVERAGE resampling options. 

- mapfile.c: avoid tail recursion in freeFeatureList().

- maplegend.c: fixed leak of imageObj when embedding legends.

- msGDALCleanup(): better error handler cleanup.

- Modified msResetErrorList() to free the last error link too, to ensure
  msCleanup() scrubs all error related memory. 

- Fix in msGetGDALGetTransform() to use default geotransform even if
  GDALGetGeoTransform() fails but alters the geotransform array.

- Typemaps for C# to enable imageObj.getBytes() method (bug 1389).

- Enable -DUSE_ZLIB via configure for compressed SVG output (bug 1307).

- maputil.c/msAddLine(): rewrite msAddLine() to call 
  msAddLineDirectly, and use realloc() in msAddLineDirectly() to optimize
  growth of shapeObjs. (bug 1432)

- msTmpFile: ensure counter is incremented to avoid duplicate
  temporary filenames. (bug 1312)

- SLD external graphic symbol format tests now for mime type
  like image/gif instead of just GIF. (bug 1430)

- Added support for OGR layers to use SQL type filers (bug 1292)

- mapio/cgiutil - fixed POST support in fastcgi mode. (bug 1259)

- mapresample.c - ensure that multi-band raw results can be
  resampled. (bug 1372)

- Add support in OGC FE for matchCase attribute on 
  PropertyIsEqual and PropertyIsLike  (bug 1416)

- Fixed sortshp.c to free shapes after processing to avoid major
  memory leak. (bug 1418)

- fixed msHTTPInit() not ever being called which prevented msHTTPCleanup()
  from properly cleaning up cUrl with curl_global_cleanup(). (bug 1417)
  
- mapsde.c: add thread locking in msSDELCacheAdd

- fixed mappool.c so that any thread can release a connection,
  not just it's allocator. (bug 1402)

- mapthread.c/h: Added TLOCK_SDE and TLOCK_ORACLE - not used yet.

- Fixed copying of layer and join items. (bug 1403)

- Fixed copying of processing directives within copy of a layer. (bug 1399)

- Problems with string initialization. (bug 1312)

- Fix svg output for multipolygons. (bug 1390)

- Added querymapObj to PHP MapScript (bug 535)


Version 4.6.0 (2005-06-14)
--------------------------

- Bug 1163 : Filter Encoding spatial operator is Intersects 
  and not Intersect.
 
- Fixed GEOS to shapeObj for multipolgon geometries.


Version 4.6.0-rc1 (2005-06-09)
------------------------------

- Bug 1375: Fixed seg fault in mapscript caused by the USE_POINT_Z_M flag.
  This flag was not carried to the mapscript Makefile(s).

- Bug 1367: Fixed PHP MapScript's symbolObj->setPoints() to correctly 
  set symbolObj->sizex/sizey

- Bug 1373: Added $layerObj->removeClass() to PHP MapScript (was already
  in SWIG MapScript)


Version 4.6.0-beta3 (2005-05-27)
--------------------------------

- Bug 1298 : enable Attribution element in wms Capabilities XML

- Bug 1354: Added a regex wrapper, allowing MapServer to build with PHP
  compiled with its builtin regex

- Bug 1364: HTML legend templates: support [if] tests on "group_name" in 
  leg_group_html blocks, and for "class_name" in leg_class_html blocks.

- Bug 1149: From WMS 1.1.1, SRS are given in individual tags in root Layer
  element.

- First pass at properly handling XML exceptions from CONNECTIONTYPE WMS
  layers. Still needs some work. (bug 1246)

- map.h/mapdraw.c: removed MAX/MIN macros in favour of MS_MAX/MS_MIN.

- Bug 1341, 1342 : Parse the unit parameter for DWithin filter request.
  Set the layer tolerance and toleranceunit with parameters parsed.

- Bug 1277 : Support of multiple logical operators in Filter Encoding.

- mapwcs.c: If msDrawRasterLayerLow() fails, ensure that the error message
  is posted as a WCS exception.

- Added experimental support for "labelcache_map_edge_buffer" metadata to
  define a buffer area with no labels around the edge of a map (bug 1353)


Version 4.6.0-beta2 (2005-05-11)
--------------------------------

- Bug 179 :  add a small buffer around the cliping rectangle to
  avoid lines around the edges.

- Finished code to convert back and forth between GEOS geometries. Buffer and
  convex hull operations are exposed in mapscript.

- fontset.fonts hash now exposed in mapscript (bug 1345).

- Bug 1336 : Retrieve distance value for DWithin filter request 
  done with line and polygon shapes/

- Bug 985 / 1015: Don't render raster layers as classified if none of
  the classes has an expression set (gdal renderer only). 

- Bug 1344: Fixed several issues in writing of inline SYMBOLS when saving
  mapfile (missing quotes around CHARACTER and other string members of SYMBOL
  object, check for NULLs, and write correct identifiers for POSITION, 
  LINECAP and LINEJOIN).


Version 4.6.0-beta1 (2005-04-26)
--------------------------------

- Bug 1305: Added support for gradient coloring in class styles

- Bug 1335 : missing call to msInitShape in function msQueryByShape

- Bug 804 : SWF output : Make sure that the layer index is consistent 
  when saving movies if some of the layers are not drawn (because the
  status is off or out of scale ...)

- Bug 1332 - shptreevis.c: fixed setting of this_rec, as the output dbf
  file was not getting any records at all.

- Fixed Makefile.vc to make .exe files depend on the DLL, so if the DLL
  fails to build, things will stop.  Avoids the need for unnecessary
  cleans on win32.  Also fixed the rule for MS_VERSION for mapscriptvars.

- Bug 1262 : the SERVICE parameter is now required for wms and wfs 
  GetCapbilities request. It is not required for other WMS requests.
  It is required for all WFS requests.

- Bug 1302 : the wfs/ows_service parameter is not used any more. The
  service is always set to WFS for WFS layers.

- Bug 791: initialize some fields in msDBFCreate() - avoids crashes in
  some circumstances.

- Bug 1329 : Apply sld named layer on all layers of the same group

- Bug 1328 : support style's width parameter for line and polygon layers.

- Bug 564: Fixed old problem with labels occasionally drawn upside down

- Bug 1325: php mapscript function $class->settext needs only 1 argument.

- Bug 1319: Fixed mutex creation (was creator-owned) in mapthread.c. win32
  issue only.

- Bug 1103: Set the default tolerance value based on the layer type.
  The default is now 3 for point and line layers and 0 for all the others.

- Bug 1244: Removing Z and M parameter from pointObj by default. A new 
  compilation option is available to active those option --enable-point-z-m.
  This gives an overall performance gain around 7 to 10%.

- Bug 1225: MapServer now requires GD 2.0.16 or more recent

- MapScript: shapeObj allocates memory for 4 value strings, shapeObj.setValue()
  lets users set values of a shapeObj.
  
- MapScript: imageObj.getBytes() replaces imageObj.write() (bugs 1176, 1064).

- Bug 1308: Correction of SQL expression generated on wfs filters for
  postgis/oracle layers.

- Bug 1304: Avoid extra white space in gml:coordinates for gml:Box.

- mapogr.c: Insure that tile index reading is restarted in 
  msOGRLayerInitItemInfo() or else fastcgi repeat requests for a layer may 
  fail on subsequent renders.

- mapogr.c: Set a real OGRPolygon spatial filter, not just an OGRLinearRing.
            Otherwise GEOS enabled OGR builds will do expensive, and 
            incorrect Intersects() tests.

- mapogr.cpp / mapprimitive.c: Optimize msAddLine() and add msAddLineDirectly()

- mapprimitive.c: Optimizations in msTransformShapeToPixel() (avoid division)

- map.h: Made MS_NINT inline assembly for win32, linux/i86.  

- mapprimitive.c: optimized msClipPolygonRect and msClipPolylineRect for
  case where the shape is completely inside the clip rect.

- Add support for SVG output. See Bug 1281 for details. 

- Bug 1231: use mimetype "image/png; mode=24bits" for 24bit png format.
  This makes it separately selectable by WMS.

- Bug 1206: Applied locking patch for expression parser for rasters.

- Bug 1273: Fixed case in msProjectPoint() were in or out are NULL and
  a failure occurs to return NULL.  Fixed problem of WMS capabilities with
  'inf' in it. 

- SLD generation bug 1150 : replacing <AND> tag to <ogc:And>  

- Fixed bug 1118 in msOWSGetLayerExtent() (mapows.c). 

- Fixed ogcfilter bug #1252  

- Turned all C++ (//) comments into C comments (bug 1238)

- mapproject.h/configure.in: Don't check for USE_PROJ_API_H anymore.  Assume 
  we have a modern PROJ.4.

- Bug 839: Fix memory leak of font name in label cache (in mapfile.c). 

- Added msForceTmpFileBase() and mapserv -tmpbase switch to allow overriding
  temporary file naming conventions.  Mainly intended to make writing 
  testscripts using mapserv easier. FrankW. 

- maporaclespatil.c: Bug fix for: #1109, #1110, #1111, #1112, #1136, #1210,
  #1211, #1212, #1213.  Support for compound polygons, fixed internal sql to
  stay more accurate for geodetic data, added the support for getextent
  function.  Added VERSION token for layer data string.

- mapimagemap.c: Preliminary implementation of support for emitting 
  MS_SYMBOL_VECTOR symbols in msDrawMarkerSymbolIM(). 

- Bug 1204: Added multi-threading support in mapthread.c.  List of connections 
  is managed within a mutex lock, and connections are only allowed to be used
  by one thread at a time. 

- Bug 1185 : php/mapscript : add constant MS_GD_ALPHA 

- Bug 1173: In HTML legend, added opt_flag support for layer groups.

- Bug 1179: added --with-warnings configure switch, overhauled warning logic.

- Bug 1168: Improve autoscaling through classification rounding issues. 

- Fixed bug writing RGB/RGBA images via GDAL output on bigendian systems. 

- Bug 1152 : Fix WMS style capabilities output for FastCGI enabled builds.

- Bug 1135 : Added support for rotating labels with the map if they were 
  rendered with some particular angle already. 

- Bug 1143 : Missing call to msInitShape.

- Fixed PHP5 support for windows : Bug 1100. 

- Correct bug 1151 : generates twice a </Mark> tag when generating an SLD.
  This was happening the style did not have a size set.

- Oracle Spatial.  Fixed problem with LayerClose function.  Added token NONE
  for DATA statement.  Thanks Valik with the hints about the LayerClose problem
  and Francois with the hints about NONE token.  

- numpoints and stylelength members of the symbol object needs to be in sync
  with the low level values after calls to setpoints ans setstyle (Bug 1137).
 
- Use doubles instead of integers in function php3_ms_symbol_setPoints 
  (Bug 1137).

- Change the output of the expression when using a wild card for
  PropertyIsLike (Bug 1107).

- Delete temporary sld file created on disk (Bug 1123)

- Fixed msFreeFileCtx() to call free() instead of gdFree() as per bug 1125. 
  Also renamed gdFreeFileCtx() to msFreeFileCtx(). 

- Ensure error stack is cleared before accepting another call in FastCGI
  mode in mapserv.c.  Bug 1122

- Support translation of all geometry types to points in mapogr.cpp (now
  also supports multipolygon, multilinestring and geometrycollection. 
  bug 1124. 

- Added support for passing OGR layer FILTER queries down to OGR via the
  SetAttributeFilter() method if prefixed with WHERE keyword.  Bug 1126.

- Fixed support for SIZEUNITS based scaling of text when map is rotated.
  Bug 1127.


Version 4.4.0 (2004-11-29)
--------------------------

- Fixed WMS GetCapabilities 1.1.0 crash when wms_style_<...>_legendurl_*
  metadata were used (bug 1096)

- WCS GetCapabilities : Added ResponsibleParty support.

- WMS GetCapabilities : Service online resource was not url encoded (bug 1093)
 
- Fixed php mapscript problem with wfs_filter selection : Bug 1092.

- Fixed encoding problem with WFS server when wfs_service_onlineresource 
  was not explicitly specified (bug 1082)

- Add trailing "?" or "&" to connection string when required in WFS
  client layers using GET method (bug 1082)

- Fixed : SLD rasters was failing when there was Spatial Filter (Bug 1087) 

- Fixed mapwfslayer.c build error when WFS was not enabled (bug 1083)

- Check that we have vsnprintf in mapimagemap.c before using it. 


Version 4.4.0-beta3 (2004-11-22)
--------------------------------

- Added tests to minimize the threat of recursion problems when evaluating
  LAYER REQUIRES or LABELREQUIRES expressions. Note that via MapScript it
  is possible to circumvent that test by defining layers with problems 
  after running prepareImage. Other things crop up in that case too (symbol 
  scaling dies) so it should be considered bad programming practice
  (bug 1059).

- Added --with-sderaster configure option. 

- Make sure that msDrawWMSLayerLow calls msDrawLayer instead of 
  msDrawRasterLayerLow directly ensuring that some logic (transparency) that 
  are in msDrawLayer are applied (bug 541).

- Force GD/JPEG outputFormatObjects to IMAGEMODE RGB and TRANSPARENT OFF
  if they are RGBA or ON.  Makes user error such as in bug 1703 less likely.

- Advertise only gd and gdal formats for wms capabilities (bug 455).

- Pass config option GML_FIELDTYPES=ALWAYS_STRING to OGR so that all GML
  attributes are returned as strings to MapServer. This is most efficient
  and prevents problems with autodetection of some attribute types (bug 1043).
  
- msOGCWKT2ProjectionObj() now uses the OGRSpatialReference::SetFromUserInput()
  method.  This allows various convenient setting options, including the 
  ability to handle ESRI WKT by prefixing the WKT string with "ESRI::". 

- Fixed GetLegendGraphic in WMS Capabilities that were missing the '?'
  or '&' separator if it was not included in wms_onlineresource (bug 1065).

- Updated WMS/WFS client and server code to lookup "ows_*" metadata names
  in addition to the default "wms_*" (or "wfs_*") metadatas (WCS was already
  implemented this way). This reduces the amount of duplication in mapfiles
  that support multiple OGC interfaces since "ows_*" metadata can be used
  almost everywhere for common metadata items shared by multiple OGC
  interfaces (bug 568).

- Added ows_service_onlineresource metadata for WMS/WFS to distinguish
  between service and GetMap/Capabilities onlineresources (bug 375).

- Added map->setSize() to PHP MapScript (bug 1066).

- Re-enabled building PHP MapScript using PHP's bundled regex/*.o. This is
  needed to build in an environment with PHP configured as an Apache DSO
  (bugs 990, 520).

- Fixed problem with raster dither support on windows (related to ascii 
  encoding pointers) (bug 722).

- Moved PHP/SWIG MapScript layer->getExtent() logic down to msLayerGetExtent()
  to avoid code duplication (bug 1051).

- Added SDE Raster drawing support (experimental).

- HTML legends: Added [leg_header_html] and [leg_footer_html] (bug 1032).

- Added "z" support in SWIG MapScript for pointObj (bug 871).

- In PHP Mpascript when using  ms_newrectobj, the members minx, miny,
  maxx, maxy are initialized to -1 (bug 788).

- Write out proper world file with remote WMS result, it was off by half
  a pixel (bug 1050).

- Send a warning in the wms capabilities if the layer status is set 
  to default (bug 638).

- Fixed PHP MapScript compile warnings: dereferencing type-punned pointer
  will break strict-aliasing rules (bug 1053).

- Added $layer->isVisible() to PHP MapScript (bug 539).

- Ported $layer->getExtent() to PHP MapScript (bug 826).

- wms_group_abstract can now be used in the capabilities (bug 754).

- If wms_stylelist is an empty string, do not output the <StyleList> tag
  for MapContexts (bug 595).

- Avoid passing FILE* to GD library by utilizing GD's gdIOCtx interface
  (bug 1047).
  
- Output warning in wms/wfs capabilities document if layer,group,map names have
  space in them (bug 486, bug 646).

- maporaclespatial.c: fixed declarations problems (bug 1044).

- Allow use of msOWSPrintURLType with no metadata. In this case the default 
  parameters will be used (bug 1001).

- Ensure the outputFormatObj attached to msImageLoadGDStream() results reflect
  the interlacedness of the loaded image.  Also ensure that the RGB PNG
  reference images work (make imagemode match gdImg) (bug 1039).

- Fixed support for non-square pixels in WCS (bug 1014).

- Expose only GD formats for GetLegendGraphic in the capabilities (bug 1001).

- Check for supported formats when process a GetLegendGraphic request 
  (bug 1030).

- mapraster.c: fixed problem with leaks in tileindexed case where the
  tile index is missing (bug 713).

- Oracle Spatial: implemented connection pool support for Oracle Spatial. 
  New layer data parameters to support query functions, added 
  "using unique <column name>".  Added "FILTER", "RELATE" and "GEOMRELATE" 
  parameters, now permit users to choose the Oracle Spatial Filter. Modified 
  the internal SQL to always apply FILTER function. And improve the Oracle 
  Spatial performance.

- Centralize "stdout binary mode setting" for win32 in msIO_needBinaryStdout().
  Use it when writing GDAL files to stdout in mapgdal.c.  Fixes problems with
  output of binary files from GDAL outputformat drivers on win32 via WMS/WCS.

- MapServer now provides one default style named (default), title and 
  LegendURL when generating capabilities.  Added also the possibility to use
  the keyword default for STYLES parameter when doing a GetMap 
  (..&STYLES=default,defeault,...) (bug 1001).

- Add xlink:type="simple" in WMS MetadataURL (bug 1027).


Version 4.4.0-beta2 (2004-11-03)
--------------------------------

- free mapServObj properly in mapserv.c in OWS dispatch case to fix minor
  memory leaks.

- modified msCloseConnections() to also close raster layers so that 
  held raster query results will be freed. 

- modified raster queries to properly set the classindex in the resultcache.

- modified msDrawQueryCache() to be very careful to not try and lookup 
  information on out-of-range classindex values.  This seems to occur when
  default shapes come back with a classindex of 0 even if there are no classes.
  (ie. raster query results). 

- the loadmapcontext function has changed it behavior.  Before the 4.4 release
  when loading layers from a map context, the layer name was built using
  a unique prefix + the name found in the context (eg for the 2nd layer in 
  map context named park, the layer name generated would possibly be l:2:park).
  Now the loadmapcontext takes a 2nd optional argument to force the creation
  of the unique names. The default behavior is now to have the layer name
  equals to the name found in the context file (bug 1023).   

- Fixed problem with WMS GetCapabilities aborting when wms_layer_group is
  used for some layers but not for all (bug 1024).

- Changed raster queries to return the list of all pixel values as an 
  attribute named "value_list" rather than "values" to avoid conflict with
  special [values] substitution rule in maptemplate.c.

- Fixed raster queries to reproject results back to map projection, and to
  do point queries distance checking against the correct projection (bug 1021).

- Get rid of WMS 1.0.8 support. It's not an officially supported version
  of the spec anyway: it's synonymous for 1.1.0 (bug 1022).

- Allow use of '=' inside HTML template tag parser (bug 978).

- Use metadata ows_schema_location for WMS/WFS/WCS/SLD (bugs 999, 1013, 938).
  The default value if metadata is not found is 
  http://schemas.opengeospatial.net.
 
- Generate a RULE <Name> tag when generating an SLD (bug 1010).

- WMS GetLegendGraphic uses now the RULE value to return an icon for
  a class that has the same name as the RULE value (bug 843).
 
- Add msOWSPrintURLType: This function is a generic URL printing function for 
  OGC specification metadata (WMS, WFS, WCS, WMC, etc.) (bug 944).

- Support MetadataURL, DataURL and LegendURL tags in WMS capabilities 
  document and MetadataURL in WFS capabilities.

- SWIG mapscript: clone methods for layerObj, classObj, styleObj (bug 1012).

- Implemented an intarray helper class for SWIG mapscript which allows for
  multi-language manipulation of layer drawing order (bugs 853, 1005).

- Fixed WMS GetLegendGraphic which was returning an exception (GD error)
  when requested layer was out of scale (bug 1006).

- Fixed maplexer.l to work with flex 2.5.31 (bug 975).

- WMS GetMap requests now have MS_NONSQUARE enabled by default. This means
  that if the width/height ratio doesn't match the extent's x/y ratio then
  the map is stretched as stated in the WMS specification (bug 862).

- In WMS, layers with no explicit projection defined will receive a copy
  of the map's projectionObj if a new SRS is specified in the GetMap request
  or if MS_NONSQUARE is enabled. This will prevent the problem with layers 
  that don't show up in WMS request when the server administrator forgets
  to explicitly set projections on all the layers in a WMS mapfile (bug 947).

- Implemented FastCGI cleanup support for win32 and unix in mapserv.c.

- Solved configure/compile issues with libiconv (bugs 909, 1017).


Version 4.4.0-beta1 (2004-10-21)
--------------------------------

- "shared" compilation target now supports some kind of versioning,
  should at least prevent libmap.so version collisions when upgrading
  MapServer on a server (bug 982).

- When no RULE parameter has been specified in the WMS request
  a legend should be returned with all classes for the specified LAYER.
  Changes has been made in mapwms.c (bug 653). Also if the SCALE parameter
  is provided in the WMS request is will be used to determine whether
  the legend of the specified layer should be drawn in the case that the
  layer is scale dependent (big 809).

- Nested layers in the capabilities are supported by using a new metadata
  tag WMS_LAYER_GROUP (bug 776).
  
- Added greyscale+alpha render support if mapdrawgdal.c (bug 965). 

- Added --with-fastcgi support to configure.

- support OGC mapcontext through mapserver cgi (bug 946).

- support for reading 3d shape file (z) (bug 869).

- add php mapscript functions to expose the z element (bug 870).

- imageObj::write() method for SWIG mapscript (bug 941).

- Protect users from 3 potential sources of threading problems: parsing
  expression strings outside of msLoadMap, evaluating mapserver logical
  expressions, and loading symbol set files outside of msLoadMap (bug 339).

- Various fixes allowing unit tests to run leak free under valgrind on 
  i686.  Memory is now properly freed when exiting from common error
  states (bug 927).

- Restored ability to render transparent (indexed or alpha) pixmap symbols
  on RGB map images, including annotation layers and embedded scalebars.
  This feature remains OFF by default for map layers and is enabled by
  specifying TRANSPARENCY ALPHA (bugs 926, 490).

- mapserv_fcgi.c removed.  Committed new comprehensive FastCGI support. 

- New mapserver exceptions for Java mapscript thanks to Umberto Nicoletti
  (bug 895).

- Removed mapindex.c, mapindex.h, shpindex.c components of old unused 
  shapefile indexing method.  

- Use the symbol size instead of 1 for the default style size value. This is
  done by setting the default size to -1 and adding msSymbolGetDefaultSize() 
  everywhere to get the default symbolsize (Bug 751).

- Correct Bug with GML BBOX output when using a <Filter> with a 
  GetFeature request (Bug 913).

- Encode all metadatas and mapfile parameters outputted in a xml document
  (Bug 802).

- Implement the ENCODING label parameter to support internationalization.
  Note this require the iconv library (Bug 858).

- New and improved Java mapscript build provided by unicoletti@prometeo.it
  and examples by Y.K. Choo (bug 876).

- MapContext: Cleanup code to make future integration more easily and output 
  SRS and DataURL in the order required by the spec.

- Fixed issue with polygon outline colors and brush caching (bug 868).

- New C# mapscript makefiles and examples provided by Y.K. Choo
  <ykchoo@geozervice.com> committed under mapscript/csharp/ (bug 867).
  
- Renamed 'string' member of labelCacheMemberObj to 'text' to avoid
  conflicts in SWIG mapscript with C# and Java types (bug 852).
  
- Fixed Bug 866 : problem when generating an sld on a pplygon layer

- SWIG mapscript: map's output image width and height should be set 
  simultaneously using new mapObj::setSize() method.  This performs
  necessary map geotransform computation.  Direct setting of map width
  and height is deprecated (bug 836).

- Fixed bug 832 (validate srs value) : When the SRS parameter in a GetMap 
  request contains a SRS that is valid for some, but not all of the layers 
  being requested, then the server shall throw a Service Exception 
  (code = "InvalidSRS"). Before this fix, mapserver use to reproject
  the layers to the requested SRS. 

- Fixed bug 834: SE_ROW_ID in SDE not initialized for unregistered SDE tables

- Fixed bug 823 : adding a validation of the SRS parameter when doing
  a GetMap request on a wms server. Here is the OGC statement :
  'When the SRS parameter in a GetMap request contains a SRS 
  that is valid for some, but not all of the layers being requested, 
  then the server shall throw a Service Exception (code = "InvalidSRS").'

- Set the background color of polygons or circles when using transparent
  PIXMAP symbol.

- SWIG mapscript class extensions are completely moved from mapscript.i
  into separate interface files under mapscript/swiginc.

- Overhaul of mapscript unit testing framework with a comprehensive test
  runner mapscript/python/tests/runtests.py.

- Modified the MS_VALID_EXTENT macro to take an extent as its argument 
  instead of the quartet of members. MapServer now checks that extents input 
  through the mapfile are valid in mapfile.c (web, map, reference, 
  and layer).  Modified msMapSetExtent in mapobject.c to use the new 
  macro instead of its home-grown version. Modified all cases that used 
  MS_VALID_EXTENT to the new use case.
  
- Layers now accept an EXTENT through the mapfile (bug 786). Nothing 
  is done with it at this point, and getExtent still queries the 
  datasource rather than getting information from the mapfile-specified 
  extent.

- Fixed problem with WMS GetFeatureInfo when map was reprojected. Was a
  problem with msProjectRect and zero-size search rectangles (bug 794)

- MapServer version now output to mapscriptvars and read by Perl Makefile.PL
  and Python setup.py (bug 795).

- Map.web, layer, and class metadata are exposed in SWIG mapscript as
  first-class objects (bug 737).

- Add support for spatial filters in the SLD (Bug 782)

- A few fixes to allow php_mapscript to work with both PHP4 and PHP5.
  PHP5 support should still be considered experimental. (bug 718)

- Fixed SDE only recognizing SE_ROW_ID as the unique column (bug 536). 
  The code now autosenses the unique row id column.

- Enhanced SDE support to include support for queries against 
  user-specified versions.  The version name can be specified as the 
  last parameter of the CONNECTION string.

- Fixed automated generation of onlineresource in OWS GetCapabilities
  when the xxx_onlineresource metadata is not specified: the map= parameter
  used to be omitted and is now included in the default onlineresource if
  it was explicitly set in QUERY_STRING (bug 643)

- Fixed possible crash when producing WMS errors INIMAGE (bug 644)

- Fixed automated generation of onlineresource in OWS GetCapabilities
  when the xxx_onlineresource metadata is not specified: the map= parameter
  used to be omitted and is now included in the default onlineresource if
  it was explicitly set in QUERY_STRING (bug 643)

- Fixed an issue with annotation label overlap. There was an issue with
  the way msRectToPolygon was computing it's bounding box. (bug 618)

- Removed "xbasewohoo" debug output when using JOINs and fixed a few 
  error messages related to MySQL joins (bug 652)

- Fixed "raster cracking" problem (bug 493)

- Improvements to Makefile.vc, and nmake.opt so that a mapscriptvars file
  can be produced on windows.  

- Updated setup.py so Python MapScript builds on win32. 

- Added preliminary raster query support.

- No more Python-stopping but otherwise benign errors raised from
  msDrawWMSLayer() (bug 650).

- Finished prototyping all MapServer functions used by SWIG-Mapscript
  and added 'void' to prototypes of no-arg functions, eliminating all
  but two SWIG-Mapscript build warnings (bug 658).

- Mapscript: resolved issue with pens and dynamic drawing of points (bug 663).

- Mapscript: fixes to tests of shape copying and new image symbols.

- Mapscript: new OWSRequest class based on cgiRequestObj structure in 
  cgiutil.h is a first step to allow programming with MapServer's OWS
  dispatching (bug 670).

- Mapscript: styles member of classObj structure is no longer exposed to
  SWIG (bug 611).

- Implementation geotransform/rotation support in cgi core, and mapscript.i.

- Testing: fixed syntax error, 'EPSG' -> 'epsg' in test.map (bug 687).
  Added an embedded scalebar which demonstrates that bug 519 is fixed.
  The test data package is also made more complete by including two fonts
  from Bitstream's open Vera fonts (bug 694).

- Mapscript (SWIG): remove promote and demote methods from layerObj.  Use
  of container's moveLayerUp/moveLayerDown is better, and this brings
  the module nearer to PHP-Mapscript (bug 692).

- mapogr.cpp: Now echos CPLGetLastErrorMsg() results if OGR open fails. 

- mapraster.c: fixed tile index corruption problem (bug 698)

- Mladen Turk's map copying macros in mapcopy.h clean up map cloning and
  allow for copying of fontset and symbolset.  Added cloning tests in
  python/tests/testCloneMap.py and refactored testing suite (bugs 640 & 701).

- Mapscript: removing obsolete python/setup_wnone.py file.

- CONFIG MS_NONSQUARE YES now enables non-square pixel mode (mostly for WMS). 
  Changes in mapdraw.c (msDrawMap()) to use the geotransform "hack" to allow
  non-square pixels.

- When using the text/html mime type in a GetFeature request, if the
  layer's template is not set to a valid file, errors occur.
  Correction is : the text/html is not advertized by default and 
  will only be advertized if the user has defined 
  "WMS_FEATURE_INFO_MIME_TYPE"  "text/html" (bug 736)

- Make PHP MapScript's layer->open() produce a PHP Warning instead of a
  Fatal error (bug 742)

- MapServer hash tables are now a structure containing a items pointer
  to hashObj. See maphash.h for new prototypes of hash table functions.
  In SWIG mapscript, Map, Layer, and Class metadata are now instances of the
  new hashTableObj class.  fontset.fonts and Map.configoptions are also
  instances of hashTableObj.  The older getMetaData/setMetaData and
  metadata iterator methods can be deprecated (bug 737).
  
- Mapscript-SWIG: made the arguments of mapObj and layerObj constructors
  optional.  A layerObj can now exist outside of a map and can be added
  to a mapObj using the insertLayer method.  mapObj.removeLayer now 
  returns a copy of the removed Layer rather than an integer (bug 759).

- Fixed $map->processTemplate() which was always returning NULL.
  Bug introduced in version 4.0 in all flavors of MapScript (bug 410)


Version 4.2-beta1 (2004-04-17)
------------------------------

- Added support for WMS 1.1.1 in the WMS interface. 

- Added support for WMS-SLD in client and server mode.

- Added support for attribute filters in the WFS interface.

- WMS Interface: several fixes to address issues found in running tests
  against the OGC testsuite. One of the side-effects is that incomplete
  GetMap requests that used to work in previous versions will produce
  errors now (see bug 622).

- Modified configure scripts to be able to configure/build PHP MapScript
  using an installed PHP instead of requiring the full source tree.

- Added ability to combine multiple WMS connections to the same server 
  into a single request when the layers are adjacent and compatible. (bug 116)

- Support POSTed requests without Content-Length set. 

- Added support for proper classification of non-8bit rasters.  

- Added support for BYTE rawmode output type. 

- Added support for multiple bands of output in rawmode. 

- MySQL joins available

- Fixed problems with detection of OGRRegisterAll() with GDAL 1.1.9 in 
  configure due to GDAL's library name change. Fixed a few other minor
  issues with GDAL/OGR in configure.

- Modified configure to disable native TIFF/PNG/JPEG/GIF support by default
  if GDAL is enabled.  You can still enable them explicitly if you like.

- Replace wms_style_%s_legendurl, wms_logourl, wms_descriptionurl, wms_dataurl 
  and wms_metadataurl metadata by four new metadata by metadata replaced. The 
  new metadata are called legendurl_width, legendurl_height, legendurl_format, 
  legendurl_href, logourl_width, etc...
  Old dependency to the metadata with four value in it , space separated, are 
  not kept.

- Implement DataURL, MetadataURL and DescriptionURL metadata in 
  mapcontext.c (bug 523)

- PHP MapScript's pasteImage() now takes a hex color value (e.g. 0xrrggbb)
  for the transparent color instead of a color index. (bug 463)

- OGR data sources with relative paths are now checked relative to 
  SHAPEPATH first, and if not found then we try again relative to the
  mapfile location.  (bug 295)

- There is a new mapObj parameter called MAXSIZE to control maximum image
  size to serve via the CGI and WMS interfaces. The default is 2048 as 
  before but it can be changed in the map file now. (bug 435)

- Added simple dataset for unit and regression tests (bug 453)

- PostGIS: added postresql_NOTICE_HANDLER() sending output via msDebug() 
  and only when layer->debug is set (bug 418)

- Added Apache version detection in configure and added non-blocking flag
  on stderr in msDebug() to work around Apache 2.x bug (bug 458)

- MapScript rectObj: added optional bounding value args to constructor and
  extended rectObj class with a toPolygon method (bug 508).
  
- MapScript pointObj: added optional x/y args to constructor (bug 508).

- MapScript colorObj: added optional RGB color value args to colorObj
  constructor, and extended colorObj class with setRGB, setHex, and toHex
  methods.  The hex methods use hex color strings like '#ffffff' rather
  than '0xffffff' for compatibility with HTML (bug 509).

- MapScript outputFormatObj: extended with a getOption method (bug 510). 

- MapScript imageObj: added optional mapObj argument to the save method
  resolving bug 549 without breaking current API.  Also added optional
  driver and filename arguments to constructor which allows imageObj
  instances to be created with a specified driver or from files on disk
  (bug 530).  Added new code to Python MapScript which extends the 
  filename option to Python file-like objects (bug 550). This means
  StringIO and urllib's network objects!

- MapScript classObj and styleObj: added a new styleObj shadow class and
  extended classObj with getStyle, insertStyle, and removeStyle methods.
  MapScript now supports multiple styles for dynamically created classes
  (bug 548).

- MapScript layerObj: added getExtent, getNumFeatures extension methods,
  allowing getShape to access inline features (bug 562).

- Added fixes for AMD64/Linux in configure (bug 565)

- Removed OGR_STATIC stuff in configure script that used to allow us to 
  build with OGR statically by pointing to the OGR source tree.  That 
  means you can only build with OGR when *installed* as part of GDAL,
  but that's what everyone is doing these days anyway.

- Mapscript outputFormatObj: extended constructor to allow format names,
  and mapObj methods to append and remove output formats from the 
  outputformatlist (bug 511).

- New SWIG mapscript development documentation in the spirit of the
  PHP-Mapscript readme file, but using reST (bug 576).

- Paving way for future changes to SWIG mapscript API with new features
  enabled by NEXT_GENERATION_API symbol (bug 586).

- Added ability to set string member variables to NULL in PHP MapScript
  (bug 591)

- New key iterators for map, layer, and class metadata hash tables 
  (bug 434) and fontset fonts hash table (bug 439).

- Fixed potential crash when using nquery with a querymap enabled and
  some layers have a template set at the layer level instead of inside
  classes (bug 569).

- New CONFIG keyword in the MAP object in a .map file to be used
  to set external configuration parameters such as PROJ_LIB and control
  of some GDAL and OGR driver behaviours (bug 619)

Version 4.0 (2003-08-01)
------------------------

- Fixed problem with truncated expressions (bugs 242 and 340)

- Attempt at fixing GD vs libiconv dependency problems (bug 348)

- Fixed problem with invalid BoundingBox tag in WMS capabilities (bug 34)

- Fixed problems with SIZEUNITS not working properly (bug 373)

- Fixed MacOSX configure problems for linking php_mapscript (bug 208)

- Fixed problem with reference map marker symbol not showing up (bug 378)

- Use <Keywords> in WMS 1.0.0 capabilities instead of <KeywordList> (bug 129)

- One-to-one and one-to-many joins now work for Xbase files and are available
  to query templates. Low level one-to-one Xbase joins are available  via
  OGR.

Version 4.0-beta2 (2003-07-11)
------------------------------

- Added prototype of FastCGI support in mapserv_fcgi.c (not built by default).

- Report full error stack in the mapserv CGI and PHP MapScript (bug 346)

- Old index (.qix) format is deprecated (bug 273)

- Fixed problem with embedded legend and scalebar that would result in
  layers being added to the HTML legends (bug 171)

- Changed joins (XBase only at this point) over to the open-prepare-next...
  next-close way of doing things. Compiles fine, but needs more testing. 
  One-to-many support should work now but it needs to be hooked into the 
  template code yet.  Last thing before a candidate 4.0 release.

- Added ability to generate images in MapScript processQueryTemplate (bug 341)

- Added saving of output formats in msSaveMap()

- Fixed problem in PHP MapScript with variables that were not dereferenced
  before their values were changed by the MapScript wrappers (bug 323)

- Added support for Web Map Context 1.0.0

- Treat zero-length template values as NULL so that it's possible to
  set("template", "") from MapScript to make layer non-queryable (bug 338)

- Ditched the shapepath argument to the shapefileObj constructor

- CARTOLINE join style default changed to MS_CJC_NONE

- Tweaked code in legend builder to handle polygon layers slightly different.
  Now if a polygon layer contains only outlines and no fills (i.e. a polyline)
  then it is drawn using the zigzag legend shape rather than the box. I'll 
  add legend outlines back in shortly.

- Restored legend key outlines (triggered by setting OUTLINECOLOR). If an 
  outline is requested then line symbols are clipped to the outline, 
  otherwise lines are allowed to bleed a pixel or two beyond those 
  boundaries- for most cases this looks	fine but for fat lines it is 
  gonna look goofy regardless. In those cases use the KEYIMAGE.

- Fixed a bug in the scanline writer so that x coordinates can be in any 
  order when passed in to the function. (bug 336)

- Updated loadExpressionString in mapfile.c to be a bit more tolerant of
  input. Now if a string does not match the logical or regex pattern it is
  automatically cast as a string expression. Removes the need for silly quotes.


Version 4.0-beta1 (2003-06-06)
------------------------------

- Added imagemap outputformat, which makes possible use of client-side 
  imagemaps in browsers.

- Added MySQL support for non-spatial OpenGIS Simple Features SQL stored data

- msQueryByShape and msQueryByFeature honor layer tolerances. In effect you
  can to buffered queries now. At the moment only polygon select features
  are supported, but there's nothing inherent in the underlying computations
  that says lines won't work as well.

- Simple one-to-one joins are working again. Reworked the join code so that
  table connections are persistent within a join (across joins is a todo).
  Joins, like layers are wrapped with a connection neutral front end, that
  sets us up to do MySQL or whatever in addition to XBase.

- Removed shapepath argument to all layer access functions (affects MapScript).
  It's still used but we leverage the layer pointer back to the parent mapObj
  so the API is cleaner.

- Changed default presentation of feature attributes to escape a few 
  problematic characters for HTML display (eg. > becomes &gt;). 
  Added [itemname_raw] substitution to allow access to unaltered data.

- Added initial version of Jan Hartman's connection pooling code.

- Replaced libwww with libcurl for WMS/WFS client HTTP requests.
  (libcurl 7.10 required, see http://curl.haxx.se/libcurl/c/)

- Added CONNECTION to the list of mapfile parameters that can accept 
  %variable% substitutions when processed by the cgi version. This is useful
  for passing in username and/or passwords to database data sources.

- Added support for DATA and TEMPLATE (header/footer/etc...) filtering using
  an regex declared in the mapfile (DATAPATTERN and TEMPLATEPATTERN). 
  Certain parameters in a mapfile cannot be changed via a URL without first
  being filtered.

- Added support for environment variable MS_MAPFILE_PATTERN. This allows you to
  override the default regex in favor of one more restrictive (I would hope) of
  your own.

- Disabled CGI SAVEMAP option.

- Removed CGI TEMPLATE option since you can use the map_web_template syntax.
  Simplifies security maintenance by only having to deal with this option 
  in a single place.

- Added offset support (styleObj) for raster based output (GD for sure, not
  quite sure how OGR output is created although I believe is uses GD anyway).
  This allows for feature drop shadows and support for cool linear symbols
  like used to be supported in pre-3.4 versions. These offsets are not 
  scalable at the moment.

- Null shapes (attributes but no vertices) are skipped for shapefiles using
  the msLayerNextShape interface. Otherwise applications should check the
  shapeObj type member for MS_SHAPE_NULL.

- Changed where label cache is allocated and cleared. Now it isn't allocated 
  until drawing takes place. Any old cache is cleared before a new one is
  allocated. The cache is still intact following rendering for post-processing
  using MapScript.

- Fixed screw up in pre-processing of logical expressions for item lists.
  Under certain circumstances that list could get corrupted and expressions
  would fail.

- Added NOT operator to expression parser.

- Added layer and map level DEBUG options to map file.

- Major changes to support vector output (PDF, SWF, GML, ...):
  imageObj is used by all rendering functions instead of gdImagePtr,
  New msSaveImage() prototype

- Support for GD-2.0, including 24 bits output.  Dropped support for GD 1.x

- Support for output to any GDAL-supported format via the new OUTPUTFORMAT
  object.

- New styleObj to replace the OVERLAY* parameter in classes.

- PostGIS: Added Sean Gillies <sgillies@i3.com>'s patch for "using unique 
  <column name>". Added "using SRID=#" to specify a spatial reference 
  for an arbitrary sql query.

- ... and numerous fixes not listed here...


Version 3.6.0-beta1 (2002-04-30)
--------------------------------

- MapScript: qitem and qstring params added to layer->queryByAttribute().
  Instead of being driven by the layer's FILTER/FILTERITEM, the query by
  attribute is now driven by the values passed via qitem,qstring, and the 
  layer's FILTER/FILTERITEM are ignored.

- Symbol and MapFile changes: ANTIALIAS and FILLED keywords now take a
  boolean (TRUE/FALSE) argument i.e. ANTIALIAS becomes ANTIALIAS TRUE
  and FILLED becomes FILLED TRUE

- Reference Map:
  Added options to show a different marker when the reference box becomes
  too small.  See the mapfile reference docs for more details on the new 
  reference object parameters (MARKER, MARKERSIZE, MAXBOXSIZE, MINBOXSIZE)

- Added MINSCALE/MAXSCALE at the CLASS level.

- Support for tiled OGR datasets.

- PHP 4.1.2 and 4.2.0 support for PHP MapScript.

- Added LAYER TRANSPARENCY, value between 1-100

- Fixes to the SWIG interface for clean Java build.

- New HTML legend templates for CGI and MapScript.  See HTML-Legend-HOWTO.

- WMS server now supports query results using HTML query templates instead
  of just plain/text.

- Added support functions for thread safety (--with-thread).  Still not 
  100% thread-safe.


Version 3.5.0 (2002-12-18)
--------------------------

- No Revision history before version 3.5


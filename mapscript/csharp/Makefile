# $Id$
#
# Run ./configure in the main MapServer directory to turn this Makefile.in
# into a proper Makefile

#
# mpatrol runtime memory debugger
#
MPATROL=  	
MPATROL_INC=	
MPATROL_LIB=	

#
# If you want to ignore missing datafile errors uncomment the following
# line. This is especially useful with large tiled datasets that may not
# have complete data for each tile.
#
#IGNORE_MISSING_DATA=-DIGNORE_MISSING_DATA
IGNORE_MISSING_DATA = -DIGNORE_MISSING_DATA

#
# If you want to use shape Z and M parameter this option must be set.
# It's OFF by default.
#
#USE_POINT_Z_M=-DUSE_POINT_Z_M
USE_POINT_Z_M = 

#
# Apparently these aren't as commonplace as I'd hoped. Edit the
# following line to reflect the missing functions on your platform.
#
# STRINGS=-DNEED_STRCASECMP -DNEED_STRNCASECMP -DNEED_STRDUP -DNEED_STRLCAT
STRINGS= -DHAVE_VSNPRINTF -DNEED_STRLCAT 

# Proj.4 distribution (cartographic projection routines). Not required for normal use. (EXPERIMENTAL)
PROJ_INC= -I/opt/fgs1/fgs-dev/built/proj/include
PROJ_LIB= -L/opt/fgs1/fgs-dev/built/proj/lib -lproj
PROJ=     -DUSE_PROJ

# GD distribution (graphics library GIF and/or PNG support). (REQUIRED)
#
#   - Versions 1.3 to 1.5 write non-LZW GIF (-DUSE_GD_1_3).
#   - Versions 1.6 and greater write PNG (-DUSE_GD_1_6). Add -lpng -lz to GD_LIB line.
#
#
GDFONT_OBJ=gd-1.2/gdfontt.o gd-1.2/gdfonts.o gd-1.2/gdfontmb.o gd-1.2/gdfontl.o gd-1.2/gdfontg.o
GD_INC=  -I/opt/fgs1/fgs-dev/built/gd//include
GD_LIB=  -L/opt/fgs1/fgs-dev/built/gd//lib -lgd   -L/opt/fgs1/fgs-dev/built/libpng/lib -lpng -lz  
GD=       -DUSE_GD_GIF -DUSE_GD_PNG -DUSE_GD_JPEG -DUSE_GD_WBMP -DUSE_GD_FT -DGD_HAS_GDIMAGEGIFPTR -DGD_HAS_GETBITMAPFONTS
GD_STATIC = 

# 
# ZLIB option (compressed SVG)
#
ZLIB_INC=  
ZLIB_LIB=  -lz
ZLIB=      -DUSE_ZLIB

# PDFlib distribution (PDF library w/ PDF support). (EXPERIMENTAL)
#
#
#
PDF_INC=  -I/opt/fgs1/fgs-dev/built/pdflib_lite/include
PDF_LIB=  -L/opt/fgs1/fgs-dev/built/pdflib_lite/lib -lpdf
PDF=      -DUSE_PDF

# TIFF distribution (raster support for TIFF and GEOTIFF imagery). (RECOMMENDED)
TIFF_INC= 
TIFF_LIB=    -L/opt/fgs1/fgs-dev/built/libpng/lib -lpng -lz  
TIFF=     

# JPEG distribution (raster support for grayscale JPEG images, INPUT ONLY).
JPEG_INC= 
JPEG_LIB= 
JPEG=     

# EPPL7 Support (this activates ERDAS as well) Included in the distribution. Probably the best raster alternative if
# you've got EPPL7 laying around. See http://www.lmic.state.mn.us/ for more information. (RECOMMENDED)
EPPL=     -DUSE_EPPL
EPPL_OBJ= epplib.o

# ESRI SDE Support. You MUST have the SDE Client libraries and include files
# on your system someplace. The actual SDE server you wish to connect to can
# be elsewhere.
SDE=	  
SDE_LIB=  
SDE_INC=  

# Optional OGR Support.  OGC Simple Feature inspired interface for vector
# formats.  See http://ogr.maptools.org/
# Note that since OGR is part of the GDAL library, it uses GDAL_LIB + GDAL_INC
OGR=	  -DUSE_OGR

# Optional GDAL Support (provides read access to a variety of raster formats)
# See http://www.remotesensing.org/gdal/
GDAL=	  -DUSE_GDAL
GDAL_LIB=  -L/opt/fgs1/fgs-dev/built/gdal/lib -lgdal
GDAL_INC=  -I/opt/fgs1/fgs-dev/built/gdal/include

# Optional GEOS Support.
# See http://geos.refractions.net/
GEOS=     
GEOS_LIB=  
GEOS_INC=  

# Optional PostGIS Support.  See http://postgis.refractions.net/
POSTGIS=      -DUSE_POSTGIS
POSTGIS_LIB=  -L/media/hda5/data-ubuntu/opt/fgs1/fgs-dev/built/postgresql/lib -lpq
POSTGIS_INC=  -I/media/hda5/data-ubuntu/opt/fgs1/fgs-dev/built/postgresql/include

# Optional MyGIS Support.  See http://postgis.refractions.net/
MYGIS=      
MYGIS_LIB=  
MYGIS_INC=  

# Optional ORACLESPATIAL Support. Contact: cabral@cttmar.univali.br
ORACLESPATIAL=
ORACLESPATIAL_LIB=
ORACLESPATIAL_INC=

# Optional MING/SWF/FLASH Support. Contact: assefa@dmsolutions.ca
MING=
MING_LIB=
MING_INC=

# libcurl ... required for WMS/WFS Client Connections
CURL_INC=-I/opt/fgs1/fgs-dev/built/curl/include
CURL_LIB=-L/opt/fgs1/fgs-dev/built/curl/lib -lcurl -L/opt/fgs1/fgs-dev/built/openssl/lib -lssl -lcrypto -ldl -lz

# Optional FastCGI
FASTCGI=
FASTCGI_INC=
FASTCGI_LIB=

# OWS: OGC Web Services support
# OGC WMS Server:  -DUSE_WMS_SVR (Requires PROJ4 support)
# OGC WMS Client Connections:  -DUSE_WMS_LYR (Requires PROJ4 and libcurl)
# OGC WFS Server: -DUSE_WFS_SVR (Requires OGR, PROJ4 recommended)
# OGC WFS Client: -DUSE_WFS_LYR (Requires OGR, PROJ4 and libcurl)
# OGC WCS Server: -DUSE_WCS_SVR (Requires GDAL and PROJ4)
OWS=-DUSE_WMS_SVR -DUSE_WMS_LYR -DUSE_WFS_SVR -DUSE_WFS_LYR 

#
# IMPORTANT NOTE ABOUT REGEX FOR PHP_MAPSCRIPT USERS:
#
# In order to compile the PHP_MAPSCRIPT module, we have to make MapServer
# uses the same version of the REGEX library that PHP was compiled with:
#
PHP_REGEX_OBJ=php_regex.o
PHP_REGEX_INC=

#
# Multithreading support.
#
THREAD=-DUSE_THREAD
THREAD_LIB=-lpthread

#
# libiconv - Enables Internationalization
#
ICONV=-DUSE_ICONV
ICONV_LIB=-L/opt/fgs1/fgs-dev/built/libiconv//lib -liconv
ICONV_INC=-I/opt/fgs1/fgs-dev/built/libiconv//include

#
# Flags.
#

XTRALIBS=  -lm -lstdc++
RUNPATHS= 

CCFLAGS= -g -O2 -fPIC -Wall  -DENABLE_STDERR_DEBUG \
        $(IGNORE_MISSING_DATA) $(USE_POINT_Z_M) $(STRINGS) $(EPPL) $(PROJ) $(OWS) $(MING) \
	$(ORACLESPATIAL) $(TIFF) $(JPEG) $(GD) $(PDF) $(SDE) $(OGR) $(GDAL) \
	$(POSTGIS) $(GEOS) $(ICONV) $(MYGIS) $(MPATROL) $(THREAD) $(ZLIB) $(GD_INC) $(PDF_INC) \
	$(PROJ_INC) $(TIFF_INC) $(JPEG_INC) $(EGIS_INC) $(SDE_INC) \
	$(REGEX_INC) $(MPATROL_INC) $(GDAL_INC) $(POSTGIS_INC) $(MYGIS_INC) \
	$(CURL_INC) $(MING_INC) $(ORACLESPATIAL_INC) $(GEOS_INC) $(ICONV_INC) \
	$(ZLIB_INC)

# Link flags and shared libs only
SUP_LIBS =  $(GD_LIB) $(PDF_LIB) $(TIFF_LIB) $(PROJ_LIB) \
          $(JPEG_LIB) $(SDE_LIB) $(GDAL_LIB) $(MING_LIB) $(POSTGIS_LIB) \
	  $(MYGIS_LIB) $(CURL_LIB) $(ORACLESPATIAL_LIB) $(GEOS_LIB) \
	  $(MPATROL_LIB) $(THREAD_LIB) $(ZLIB_LIB) $(ICONV_LIB) $(XTRALIBS)

# STATIC_LIBS is full filename with path of libs that will be statically linked
STATIC_LIBS= $(GD_STATIC)

LDFLAGS= $(RUNPATHS) -L../.. -lmap $(SUP_LIBS) $(STATIC_LIBS)

# END OF CONFIGURE----

#
# SWIG Stuff
#
SWIG= swig

#
# CSHARP Stuff
#
CSC= mcs

CC= gcc

#
# --- You shouldn't have to edit anything else. ---
#
all: interface mapscript_so mapscript_csharp

interface: ../mapscript.i
	$(SWIG) -csharp -o mapscript_wrap.c ../mapscript.i

mapscript_so: mapscript_wrap.c
	$(CC) -fpic -c $(CCFLAGS) mapscript_wrap.c
	$(CC) -fpic -shared mapscript_wrap.o -o libmapscript.so $(LDFLAGS)

mapscript_csharp::
	$(CSC) /t:library /out:mapscript_csharp.dll *.cs
	$(CSC) /r:mapscript_csharp.dll examples/shpdump.cs
	$(CSC) /r:mapscript_csharp.dll examples/shapeinfo.cs
	$(CSC) /r:mapscript_csharp.dll examples/drawmap.cs
	
clean:
	rm -rf edu *.o *.c *.cs *.so *.dll

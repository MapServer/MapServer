# Run ./configure in the main MapServer directory to turn this Makefile.in
# into a proper Makefile

prefix		=	/usr/local
exec_prefix	=	${prefix}
INST_PREFIX	=	${prefix}
INST_LIB	=	${exec_prefix}/lib
INST_BIN	=	${exec_prefix}/bin

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
# Apparently these aren't as commonplace as I'd hoped. Edit the
# following line to reflect the missing functions on your platform.
#
# STRINGS=-DNEED_STRCASECMP -DNEED_STRNCASECMP -DNEED_STRDUP -DNEED_STRLCAT
STRINGS= -DNEED_STRLCAT 

# Proj.4 distribution (cartographic projection routines). Not required for normal use. (EXPERIMENTAL)
PROJ_INC= -I/usr/local/include
PROJ_LIB= -L/usr/local/lib -lproj
PROJ=     -DUSE_PROJ -DUSE_PROJ_API_H

# GD distribution (graphics library GIF and/or PNG support). (REQUIRED)
#
#   - Versions 1.3 to 1.5 write non-LZW GIF (-DUSE_GD_1_3).
#   - Versions 1.6 and greater write PNG (-DUSE_GD_1_6). Add -lpng -lz to GD_LIB line.
#
#
GDFONT_OBJ=gd-1.2/gdfontt.o gd-1.2/gdfonts.o gd-1.2/gdfontmb.o gd-1.2/gdfontl.o gd-1.2/gdfontg.o
GD_INC=  -I/usr/local/include
GD_LIB=  -L/usr/local/lib -lgd -ljpeg -lfreetype -lpng -lz  
GD=       -DUSE_GD_GIF -DUSE_GD_PNG -DUSE_GD_JPEG -DUSE_GD_WBMP -DUSE_GD_FT -DGD_HAS_GDIMAGEGIFPTR
GD_STATIC = 

# PDFlib distribution (PDF library w/ PDF support). (EXPERIMENTAL)
#
#
#
PDF_INC=  
PDF_LIB=  
PDF=      

# TIFF distribution (raster support for TIFF and GEOTIFF imagery). (RECOMMENDED)
TIFF_INC= 
TIFF_LIB=  -ljpeg -lfreetype -lpng -lz  
TIFF=     

# JPEG distribution (raster support for grayscale JPEG images, INPUT ONLY).
JPEG_INC= 
JPEG_LIB= -ljpeg
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
GDAL_LIB=  -L/usr/local/lib -lgdal
GDAL_INC=  -I/usr/local/include

# Optional PostGIS Support.  See http://postgis.refractions.net/
POSTGIS=      -DUSE_POSTGIS
POSTGIS_LIB=  -L/usr/local/pgsql_743/lib -lpq
POSTGIS_INC=  -I/usr/local/pgsql_743/include

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
CURL_INC=
CURL_LIB=-L/usr/lib -lcurl -L/usr/kerberos/lib -lssl -lcrypto -lgssapi_krb5 -lkrb5 -lcom_err -lk5crypto -lresolv -ldl -lz -L/usr/kerberos/lib -lssl -lcrypto -lssl -lcrypto -lgssapi_krb5 -lkrb5 -lcom_err -lk5crypto -lresolv -ldl -lz -lz

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
REGEX_OBJ=
REGEX_INC=

#
# Multithreading support.
#
THREAD=-DUSE_THREAD
THREAD_LIB=-lpthread

#
# Pick a compiler, etc. Flex and bison are only required if you need to modify the mapserver lexer (maplexer.l) or expression parser (mapparser.y).
#
CXX=	g++
CC=     gcc
LD=     gcc
AR= ar rc
RANLIB= ranlib
LEX=    flex
YACC=   bison -y
LD_SHARED = g++ -shared

XTRALIBS=  -lm -lstdc++
RUNPATHS= 

CCFLAGS= -g -O2 -fPIC -Wall -DENABLE_STDERR_DEBUG -DNEED_NONBLOCKING_STDERR \
        $(IGNORE_MISSING_DATA) $(STRINGS) $(EPPL) $(PROJ) $(OWS) $(MING) \
	$(ORACLESPATIAL) $(TIFF) $(JPEG) $(GD) $(PDF) $(SDE) $(OGR) $(GDAL) \
	$(POSTGIS) $(MYGIS) $(MPATROL) $(THREAD) $(GD_INC) $(PDF_INC) \
	$(PROJ_INC) $(TIFF_INC) $(JPEG_INC) $(EGIS_INC) $(SDE_INC) \
	$(REGEX_INC) $(MPATROL_INC) $(GDAL_INC) $(POSTGIS_INC) $(MYGIS_INC) \
	$(CURL_INC) $(MING_INC) $(ORACLESPATIAL_INC)

# Link flags and shared libs only
SUP_LIBS =  $(GD_LIB) $(PDF_LIB) $(TIFF_LIB) $(PROJ_LIB) \
          $(JPEG_LIB) $(SDE_LIB) $(GDAL_LIB) $(MING_LIB) $(POSTGIS_LIB) \
	  $(MYGIS_LIB) $(CURL_LIB) $(ORACLESPATIAL_LIB) \
	  $(MPATROL_LIB) $(THREAD_LIB) $(XTRALIBS)

# STATIC_LIBS is full filename with path of libs that will be statically linked
STATIC_LIBS= $(GD_STATIC)

EXE_LDFLAGS =	$(RUNPATHS) -L. -lmap $(SUP_LIBS) $(STATIC_LIBS)

LDFLAGS= $(RUNPATHS) -L../.. -lmap $(SUP_LIBS) $(STATIC_LIBS)

# END OF CONFIGURE----

#
# SWIG Stuff
#
SWIG=swig

#
# Java Stuff
#
JAVAC=javac
JAVA=java
JAR=jar
JAVA_INCLUDE=-I${JAVA_HOME}/include -I${JAVA_HOME}/include/linux

CC=gcc

#
# --- You shouldn't have to edit anything else. ---
#
all: mapscript_so mapscript_java mapscript_jar

# This target is for users who are generating their own interface
# with SWIG versions lower than 1.3.19 (no -outdir)
interface: ../mapscript.i
	mkdir -p edu/umn/gis/mapscript 
	$(SWIG) -java -package edu.umn.gis.mapscript -o mapscript_wrap.c ../mapscript.i
	mv *.java edu/umn/gis/mapscript/
	
# This target will be used in the release script
interface_release: ../mapscript.i
	mkdir -p edu/umn/gis/mapscript 
	$(SWIG) -java -package edu.umn.gis.mapscript -outdir edu/umn/gis/mapscript -o mapscript_wrap.c ../mapscript.i

mapscript_so: mapscript_wrap.c
	$(CC) -fpic -c $(CCFLAGS) $(JAVA_INCLUDE) -fno-strict-aliasing mapscript_wrap.c
	$(CC) -fpic -shared mapscript_wrap.o -o libmapscript.so $(LDFLAGS)

mapscript_java::
	$(JAVAC) edu/umn/gis/mapscript/*.java
	
mapscript_jar::
	$(JAR) cf mapscript.jar edu

clean:
	rm -rf edu *.o *.c *.so *.java *.jar *.png
	rm -rf examples/*.class

test:
	$(JAVAC) -classpath ./:examples/:./mapscript.jar -d examples/ examples/*.java
	$(JAVA) -classpath ./:examples/:./mapscript.jar -Djava.library.path=. DumpShp ../../tests/point.shp
	$(JAVA) -classpath ./:examples/:./mapscript.jar -Djava.library.path=. ShapeInfo ../../tests/point.shp ../../tests/point.dbf
	$(JAVA) -classpath ./:examples/:./mapscript.jar -Djava.library.path=. DrawMap ../../tests/test.map ./map.png

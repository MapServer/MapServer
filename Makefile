# Generated automatically from Makefile.in by configure.
# Run ./configure in the main MapServer directory to turn this Makefile.in
# into a proper Makefile

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
# STRINGS=-DNEED_STRCASECMP -DNEED_STRNCASECMP -DNEED_STRDUP
STRINGS= 

# Freetype distribution (TrueType font support). Available at http://www.freetype.org/. (RECOMMENDED)
TTF_LIB=   -L/export/stlime/mapserver/gdft -lgdft -lttf
TTF_INC=   -I/export/stlime/mapserver/gdft 
TTF=       -DUSE_TTF
MAKE_GDFT= gdft
MAKE_GDFT_CLEAN= gdft_clean

# Proj.4 distribution (cartographic projection routines). Not required for normal use. (EXPERIMENTAL)
PROJ_INC= 
PROJ_LIB= 
PROJ=     

# GD distribution (graphics library GIF and/or PNG support). (REQUIRED)
#
#   - Version 1.2 is included and writes LZW GIF (-DUSE_GD_1_2).
#   - Versions 1.3 to 1.5 write non-LZW GIF (-DUSE_GD_1_3).
#   - Versions 1.6 and greater write PNG (-DUSE_GD_1_6). Add -lpng -lz to GD_LIB line.
#
# If you're not using the 1.2 version included in the MapServer distribution 
# then comment out the MAKE_GD and MAKE_GD_CLEAN entries.
#
GDFONT_OBJ=gd-1.2/gdfontt.o gd-1.2/gdfonts.o gd-1.2/gdfontmb.o gd-1.2/gdfontl.o gd-1.2/gdfontg.o
GD_INC=  -I/export/stlime/mapserver/gd-1.2
GD_LIB=  -L/export/stlime/mapserver/gd-1.2 -lgd
GD=      -DUSE_GD_1_2
MAKE_GD= gd
MAKE_GD_CLEAN = gd_clean

# TIFF distribution (raster support for TIFF and GEOTIFF imagery). (RECOMMENDED)
TIFF_INC= 
TIFF_LIB= -ltiff
TIFF=     -DUSE_TIFF

# JPEG distribution (raster support for grayscale JPEG images, INPUT ONLY).
JPEG_INC= 
JPEG_LIB= -ljpeg
JPEG=     -DUSE_JPEG

# EPPL7 Support (this activates ERDAS as well) Included in the distribution. Probably the best raster alternative if
# you've got EPPL7 laying around. See http://www.lmic.state.mn.us/ for more information. (RECOMMENDED)
EPPL=     -DUSE_EPPL
EPPL_OBJ= epplib.o

# ESRI SDE Support. You MUST have the SDE Client libraries and include files
# on your system someplace. The actual SDE server you wish to connect to can
# be elsewhere.
#SDE=-DUSE_SDE
#SDE_HOME=/opt/arcsde801
#SDE_LIB=-L$(SDE_HOME)/lib -lsde30 -lpe -lsg -lpthread -lsocket
#SDE_INC=-I$(SDE_HOME)/include

#
# UofMN GIS/Image Processing Extension (very experimental)
#
#EGIS=-DUSE_EGIS
#EGIS_INC=-I./egis/errLog -I./egis/imgSrc -I./egis
#EGIS_LIB=-L./egis/errLog -lerrLog -L./egis/imgSrc -limgGEN -L./egis -legis
#MAKE_EGIS=egis
#MAKE_EGIS_CLEAN=egis_clean                                                     
#
# Pick a compiler, etc. Flex and bison are only required if you need to modify the mapserver lexer (maplexer.l) or expression parser (mapparser.y).
#
CC=     gcc
AR= ar rc
RANLIB= ranlib
LEX=    flex
YACC=   bison -y

XTRALIBS=  -lm

CFLAGS= -O2  -Wall $(IGNORE_MISSING_DATA) $(STRINGS) $(EPPL) $(PROJ) $(TTF) \
        $(TIFF) $(JPEG) $(GD) $(EGIS) $(PROJ_INC) $(GD_INC) $(TTF_INC) 	\
	$(TIFF_INC) $(JPEG_INC) $(EGIS_INC)

LDFLAGS= -L. -lmap $(GD_LIB) $(TTF_LIB) $(TIFF_LIB) $(PROJ_LIB) $(JPEG_LIB) \
	$(EGIS_LIB) $(XTRALIBS)

RM= /bin/rm -f

OBJS= mapbits.o maphash.o mapshape.o mapxbase.o mapparser.o maplexer.o mapindex.o maptree.o mapsearch.o mapstring.o mapsymbol.o mapfile.o maplegend.o maputil.o mapscale.o mapquery.o maplabel.o maperror.o mapprimitive.o mapproject.o mapraster.o mapsde.o $(EPPL_OBJ)

#
# --- You shouldn't have to edit anything else. ---
#
.c.o: 
	$(CC) -c $(CFLAGS) $<

all: $(MAKE_GD) $(MAKE_EGIS) $(MAKE_GDFT) libmap.a shp2img sym2img legend \
	mapserv shpindex shptree scalebar sortshp perlvars 

gd::
	cd gd-1.2; make; cd ..

gdft::
	cd gdft; make; cd ..

egis::
	cd egis/errLog; make; cd ..
	cd egis/imgSrc; make; cd ..
	cd egis; make; cd ..

php3_mapscript::
	cd mapscript/php3; make; cd ../..

maplexer.o: maplexer.c map.h mapfile.h

maplexer.c: maplexer.l
	$(LEX) -i -omaplexer.c maplexer.l

mapparser.o: mapparser.c map.h

mapparser.c: mapparser.y
	$(YACC) -d -omapparser.c mapparser.y

lib: libmap.a
libmap: libmap.a
libmap.a: map.h $(OBJS) map.h
	$(AR) libmap.a $(OBJS)
	$(RANLIB) libmap.a

shp2img: libmap.a  shp2img.o map.h
	$(CC) $(CFLAGS) shp2img.o $(LDFLAGS) -o shp2img

sym2img: libmap.a   sym2img.o map.h
	$(CC) $(CFLAGS) sym2img.o $(LDFLAGS) -o sym2img

legend: libmap.a  legend.o map.h
	$(CC) $(CFLAGS) legend.o $(LDFLAGS) -o legend

scalebar: libmap.a  scalebar.o map.h
	$(CC) $(CFLAGS) scalebar.o $(LDFLAGS) -o scalebar

mapserv: mapserv.h libmap.a  mapserv.o cgiutil.o map.h
	$(CC) $(CFLAGS) mapserv.o cgiutil.o  $(LDFLAGS) -o mapserv

shpindex: libmap.a shpindex.o map.h
	$(CC) $(CFLAGS) shpindex.o $(LDFLAGS) -o shpindex

shptree: libmap.a shptree.o map.h
	$(CC) $(CFLAGS) shptree.o $(LDFLAGS) -o shptree

sortshp: sortshp.o
	$(CC) $(CFLAGS) sortshp.o $(LDFLAGS) -o sortshp

perlvars:
	touch perlvars
	pwd > perlvars
	echo $(IGNORE_MISSING_DATA) $(STRINGS) $(EPPL) $(PROJ) $(TTF) $(TIFF) $(JPEG) $(GD) >> perlvars
	echo -I. $(PROJ_INC) $(GD_INC) $(TTF_INC) $(TIFF_INC) $(JPEG_INC) >> perlvars
	echo $(LDFLAGS) >> perlvars

gd_clean:
	cd gd-1.2; make clean; cd ..
 
gdft_clean:
	cd gdft; make clean; cd ..
 
egis_clean:
	cd egis/errLog; make clean; cd ..
	cd egis/imgSrc; make clean; cd ..
	cd egis; make clean; cd ..

php3_mapscript_clean::
	cd mapscript/php3; make clean; cd ../..

clean:	$(MAKE_GD_CLEAN) $(MAKE_GDFT_CLEAN) $(MAKE_EGIS_CLEAN) 
	rm -f libmap.a *.o shp2img mapserv sym2img legend shpindex shptree scalebar sortshp perlvars

sorta-clean:
	rm -f *.o

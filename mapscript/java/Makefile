
#
# SWIG Stuff
#
SWIG=/usr/local/bin/swig

#
# Java Stuff
#
JAVAC=/usr/local/java/bin/javac
JAR=/usr/local/java/bin/jar
JAVA_INCLUDE=-I/usr/local/java/include -I/usr/local/java/include/linux

CC=gcc

#
# Set these to the values appropriate for your MapServer build- cut & paste from ../../perlvars
#
CCFLAGS=-DIGNORE_MISSING_DATA -DUSE_EPPL -DUSE_TIFF -DUSE_JPEG -DUSE_GD_TTF -DUSE_GD_GIF -DUSE_GD_PNG -DUSE_GD_JPEG -DUSE_GD_WBMP -DGD_HAS_GDIMAGEGIFPTR
LDFLAGS=-L../.. -lmap -lgd -L/usr/local/lib -lgd -ljpeg -lttf -lpng -lz -ltiff -ljpeg -lfreetype -lpng -lz -ljpeg -lm

#
# --- You shouldn't have to edit anything else. ---
#
all: interface mapscript_so mapscript_java

interface: mapscript.i
	$(SWIG) -java -shadow -package edu.umn.gis.mapscript mapscript.i

mapscript_so: mapscript_wrap.c
	$(CC) -fpic -c $(CCFLAGS) $(JAVA_INCLUDE) mapscript_wrap.c
	$(CC) -fpic -shared mapscript_wrap.o -o libmapscript.so $(LDFLAGS)

mapscript_java::
	$(JAVAC) -d . *.java
	
mapscript_jar::
	$(JAR) cf mapscript.jar edu

clean:
	rm -rf edu *.o *.c *.so *.java *.jar

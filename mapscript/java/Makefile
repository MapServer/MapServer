
#
# SWIG Stuff
#
SWIG=/usr/local/bin/swig

#
# Java Stuff
#
JAVAC=/usr/local/java/bin/javac
JAVA_INCLUDE=-I/usr/local/java/include -I/usr/local/java/include/linux

CC=gcc

#
# Set these to the values appropriate for your MapServer build- cut & paste from ../../perlvars
#
CCFLAGS=-DIGNORE_MISSING_DATA -DUSE_EPPL -DUSE_TIFF -DUSE_JPEG -DUSE_GD_GIF -DUSE_GD_PNG -DUSE_GD_JPEG -DUSE_GD_WBMP -DGD_HAS_GDIMAGEGIFPTR
LDFLAGS=-L../.. -lmap -lgd -L/usr/local/lib -lgd -ljpeg -lfreetype -lpng -lz -ltiff -ljpeg -lfreetype -lpng -lz -ljpeg -lm

#
# --- You shouldn't have to edit anything else. ---
#
all: interface mapscript_so mapscript_java

interface: mapscript.i
	$(SWIG) -java -shadow -package mapscript mapscript.i
	mkdir mapscript
	mv *.java mapscript

mapscript_so: mapscript_wrap.c
	$(CC) -fpic -c $(CCFLAGS) $(JAVA_INCLUDE) mapscript_wrap.c
	$(CC) -fpic $(LDFLAGS) -shared mapscript_wrap.o -o libmapscript.so

mapscript_java::
	javac mapscript/*.java
	
clean:
	rm -rf mapscript *.o *.c *.so

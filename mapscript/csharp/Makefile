
#
# SWIG Stuff
#
SWIG=/usr/local/bin/swig

#
# CSHARP Stuff
#
CSC=mcs

CC=gcc

#
# Set these to the values appropriate for your MapServer build- cut & paste from ../../perlvars
#
CCFLAGS=-DIGNORE_MISSING_DATA -DUSE_EPPL -DUSE_TIFF -DUSE_JPEG -DUSE_GD_TTF -DUSE_GD_GIF -DUSE_GD_PNG -DUSE_GD_JPEG -DUSE_GD_WBMP -DGD_HAS_GDIMAGEGIFPTR

LDFLAGS=-L../.. -lmap -lgd -L/usr/local/lib -lgd -ljpeg -lttf -lpng -lz -ljpeg -lpng -lz -ljpeg -lm

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
	rm -rf edu *.o *.c *.so *.dll

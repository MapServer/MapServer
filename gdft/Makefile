# Generated automatically from Makefile.in by configure.
# Run ./configure in the main MapServer directory to turn this Makefile.in
# into a proper Makefile

# GD distribution (graphics library GIF and/or PNG support).
#
#   - Version 1.2 writes LZW GIF (-DUSE_GD_1_2).
#   - Versions 1.3 to 1.5 write non-LZW GIF (-DUSE_GD_1_3).
#   - Versions 1.6 and greater write PNG (-DUSE_GD_1_6). Add -lpng -lz to GD_LIB line.
#
GD_INC=  -I/export/stlime/mapserver/gd-1.2
GD_LIB=  -L/export/stlime/mapserver/gd-1.2 -lgd
GD=      -DUSE_GD_1_2

TTF_INC= -I/export/stlime/mapserver/gdft 
TTF_LIB= -L/export/stlime/mapserver/gdft -lgdft -lttf
TTF=     -DUSE_TTF

#
# Pick a compiler, etc...
#
CC=      gcc
AR= ar rc
RANLIB=  ranlib
LEX=     flex

XTRALIBS=  -lm

CFLAGS= -O2  -Wall $(GD) $(GD_INC) $(TTF) $(TTF_INC)
LDFLAGS= $(XTRALIBS) $(GD_LIB) $(TTF_LIB)

RM= /bin/rm -f

OBJS= gdkanji.o gdcache.o  gdttf.o

#
# --- You shouldn't have to edit anything else. ---
#
.c.o: 
	$(CC) -c $(CFLAGS) $<

all: lib

lib: libgdft.a
libgdft: libgfft.a
libgdft.a: $(OBJS)
	$(AR) libgdft.a $(OBJS)
	$(RANLIB) libgdft.a

clean:
	rm -f libgdft.a *.o

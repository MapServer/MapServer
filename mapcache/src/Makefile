include ../Makefile.inc

LIBTOOL=../libtool

all: $(TARGETS)

install-module: mod_geocache.o
	$(APXS) -i -a -n geocache mod_geocache.la

install-fcgi: geocache.fcgi fcgihelp

fcgihelp:
	echo "fcgi help"

.c.o:
	$(LIBTOOL) --mode=compile $(CC) $(GC_CFLAGS) $(APR_CFLAGS) -c $<
	
mod_geocache.la: mod_geocache.o

mod_geocache.o:  mod_geocache.c $(OBJS)
	$(APXS) $(APXS_WCFLAGS) $(GC_LIBS) -c mod_geocache.c $(LOBJS)
	
geocache.fcgi: fastcgi_geocache.c $(OBJS)
	$(LIBTOOL) --mode=link $(CC) -o geocache.fcgi $(GC_CFLAGS) $(APR_CFLAGS) $(FCGI_INC) fastcgi_geocache.c $(LOBJS) $(GC_LIBS) $(APR_LIBS) $(FCGI_LIB)
	

clean:
	rm -f *.o
	rm -f *.so
	rm -f *.slo
	rm -f *.lo
	rm -rf .libs
	rm -f *.la
	rm -f *.sla
	rm -f geocache.fcgi


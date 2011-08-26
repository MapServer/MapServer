include ../Makefile.inc

all: $(TARGETS)

install-module: mod_geocache.o
	$(APXS) -i -a -n geocache mod_geocache.la

install-fcgi: geocache.fcgi fcgihelp

fcgihelp:
	echo "fcgi help"

.c.o:
	$(LIBTOOL) --tag CC --mode=compile $(CC) $(CFLAGS) $(INCLUDES) -c $<
	
mod_geocache.la: mod_geocache.o

mod_geocache.o:  mod_geocache.c $(OBJS)
	$(APXS) "-Wc,$(CFLAGS) $(INCLUDES)" "-Wl,$(LIBS)" -c mod_geocache.c $(LOBJS)
	
geocache.fcgi: fastcgi_geocache.c $(OBJS)
	$(LIBTOOL) --mode=link --tag CC $(CC) -o geocache.fcgi $(CFLAGS) $(INCLUDES) $(FCGI_INC) fastcgi_geocache.c $(LOBJS) $(LIBS) $(FCGI_LIB)

geocache_seed: geocache_seed.c $(OBJS)
	$(LIBTOOL) --mode=link --tag CC $(CC) -o geocache_seed $(CFLAGS) $(INCLUDES) geocache_seed.c $(LOBJS) $(LIBS) 


clean:
	rm -f *.o
	rm -f *.so
	rm -f *.slo
	rm -f *.lo
	rm -rf .libs
	rm -f *.la
	rm -f *.sla
	rm -f geocache.fcgi


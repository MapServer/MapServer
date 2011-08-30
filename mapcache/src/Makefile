include ../Makefile.inc

all: $(TARGETS)

install-module: mod_mapcache.o
	$(APXS) -i -a -n mapcache mod_mapcache.la

install-fcgi: mapcache.fcgi fcgihelp

fcgihelp:
	echo "fcgi help"

.c.o:
	$(LIBTOOL) --tag CC --mode=compile $(CC) $(CFLAGS) $(INCLUDES) -c $<
	
mod_mapcache.la: mod_mapcache.o

mod_mapcache.o:  mod_mapcache.c $(OBJS)
	$(APXS) "-Wc,$(CFLAGS) $(INCLUDES)" "-Wl,$(LIBS)" -c mod_mapcache.c $(LOBJS)
	
mapcache: fastcgi_mapcache.c $(OBJS)
	$(LIBTOOL) --mode=link --tag CC $(CC) -o mapcache $(CFLAGS) $(INCLUDES) $(FASTCGI_ENABLED) $(FASTCGI_INC) fastcgi_mapcache.c $(LOBJS) $(LIBS) $(FASTCGI_LIB)

mapcache_seed: mapcache_seed.c $(OBJS)
	$(LIBTOOL) --mode=link --tag CC $(CC) -o mapcache_seed $(CFLAGS) $(INCLUDES) mapcache_seed.c $(LOBJS) $(LIBS) $(SEEDER_EXTRALIBS)


clean:
	rm -f *.o
	rm -f *.so
	rm -f *.slo
	rm -f *.lo
	rm -rf .libs
	rm -f *.la
	rm -f *.sla
	rm -rf *.dSYM
	rm -f mapcache.fcgi
	rm -f mapcache_seed


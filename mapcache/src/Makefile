include ../Makefile.inc

all: $(TARGETS)

install-lib: libmapcache.la
	$(LIBTOOL) --mode=install $(INSTALL) libmapcache.la $(libdir)

install-module: mod_mapcache.o
	$(APXS) -i -a -n mapcache mod_mapcache.la

.c.o:
	$(LIBTOOL) --tag CC --mode=compile $(CC) $(ALL_ENABLED) $(CFLAGS) $(INCLUDES) -c $<

libmapcache.la: $(OBJS)
	$(LIBTOOL) --mode=link --tag CC $(CC) -rpath $(libdir) $(LOBJS) $(LIBS)-o $@

mod_mapcache.la: mod_mapcache.o

mod_mapcache.o:  mod_mapcache.c libmapcache.la
	$(APXS) -Wc,"$(ALL_ENABLED) $(CFLAGS) $(INCLUDES)" -Wl,"$(LIBS)" -c mod_mapcache.c libmapcache.la
	
mapcache: fastcgi_mapcache.c libmapcache.la
	$(LIBTOOL) --mode=link --tag CC $(CC) -rpath $(bindir) -o mapcache $(CFLAGS) $(ALL_ENABLED) $(INCLUDES) $(FASTCGI_ENABLED) $(FASTCGI_INC) fastcgi_mapcache.c libmapcache.la $(LIBS) $(FASTCGI_LIB)

mapcache_seed: mapcache_seed.c libmapcache.la
	$(LIBTOOL) --mode=link --tag CC $(CC) -rpath $(bindir) -o mapcache_seed $(ALL_ENABLED) $(CFLAGS) $(INCLUDES) $(SEEDER_EXTRAINC) mapcache_seed.c libmapcache.la $(LIBS) $(SEEDER_EXTRALIBS)

install: install-lib mapcache mapcache_seed
	$(LIBTOOL) --mode=install $(INSTALL) mapcache $(bindir)
	$(LIBTOOL) --mode=install $(INSTALL) mapcache_seed $(bindir)

clean:
	rm -f *.o
	rm -f *.so
	rm -f *.slo
	rm -f *.lo
	rm -rf .libs
	rm -f *.la
	rm -f *.sla
	rm -rf *.dSYM
	rm -f mapcache
	rm -f mapcache_seed


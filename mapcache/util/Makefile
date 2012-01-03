include ../Makefile.inc

all: mapcache_seed

.c.o:
	$(LIBTOOL) --tag CC --mode=compile $(CC) $(ALL_ENABLED) $(CFLAGS) $(INCLUDES) -c $<

mapcache_seed: mapcache_seed.c ../lib/libmapcache.la
	$(LIBTOOL) --mode=link --tag CC $(CC) -rpath $(bindir) -o mapcache_seed $(ALL_ENABLED) $(CFLAGS) $(INCLUDES) $(SEEDER_EXTRAINC) mapcache_seed.c ../lib/libmapcache.la $(LIBS) $(SEEDER_EXTRALIBS)

install: mapcache_seed
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
	rm -f mapcache_seed


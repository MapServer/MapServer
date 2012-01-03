include ../Makefile.inc

all: mod_mapcache.la


install: mod_mapcache.la
	$(APXS) -i -a -n mapcache mod_mapcache.la

.c.o:
	$(LIBTOOL) --tag CC --mode=compile $(CC) $(ALL_ENABLED) $(CFLAGS) $(INCLUDES) -c $<

mod_mapcache.la:  mod_mapcache.c ../lib/libmapcache.la
	$(APXS) -Wc,"$(ALL_ENABLED) $(CFLAGS) $(INCLUDES)" -Wl,"$(LIBS)" -c mod_mapcache.c ../lib/libmapcache.la
	
clean:
	rm -f *.o
	rm -f *.so
	rm -f *.slo
	rm -f *.lo
	rm -rf .libs
	rm -f *.la
	rm -f *.sla
	rm -rf *.dSYM


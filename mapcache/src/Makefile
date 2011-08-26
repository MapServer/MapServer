include ../Makefile.inc

all: .libs/mod_geocache.so

install: .libs/mod_geocache.so
	$(APXS) -i -a -n geocache mod_geocache.la

.c.o:
	$(APXS) -Wc,"$(CFLAGS)" -I../include $(CAIRO_INC) $(MERGE_ENABLED) $(CAIRO_ENABLED) $(CURL_CFLAGS) $(XML2_CFLAGS) -c $<
	
mod_geocache.o:  mod_geocache.c $(OBJS)
	$(APXS) -Wc,"$(CFLAGS)" -I../include $(CAIRO_INC) $(MERGE_ENABLED) $(CAIRO_ENABLED) $(CURL_CFLAGS) $(XML2_CFLAGS) $(XML2_LIBS) $(CURL_LIBS) $(CAIRO_LIB) -c mod_geocache.c $(LOBJS)
	
.libs/mod_geocache.so: mod_geocache.o $(OBJS)


clean:
	rm -f *.o
	rm -f *.so
	rm -f *.slo
	rm -f *.lo
	rm -rf .libs
	rm -f *.la
	rm -f *.sla


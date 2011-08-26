include ../Makefile.inc

all: mod_geocache.so geocache.fcgi

install: mod_geocache.so
	$(APXS) -i -a -n geocache mod_geocache.so

.c.o:
	$(CC) $(ALL_CFLAGS) -c $<
	
mod_geocache.o:  mod_geocache.c $(OBJS)
	$(CC) $(ALL_CFLAGS) -c mod_geocache.c
	
mod_geocache.so: mod_geocache.o
	$(CC) -shared -o mod_geocache.so mod_geocache.o $(OBJS) $(ALL_LIBS) -Wl,-soname -Wl,mod_geocache.so
	
geocache.fcgi: fastcgi_geocache.c $(OBJS)
	$(CC) -o geocache.fcgi $(ALL_CFLAGS) $(FCGI_INC) fastcgi_geocache.c $(OBJS) $(ALL_LIBS) $(FCGI_LIB)
	

clean:
	rm -f *.o
	rm -f *.so
	rm -f *.slo
	rm -f *.lo
	rm -rf .libs
	rm -f *.la
	rm -f *.sla
	rm -f geocache.fcgi


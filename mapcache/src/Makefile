include ../Makefile.inc

all: .libs/mod_geocache.so geocache.fcgi

install: .libs/mod_geocache.so
	$(APXS) -i -a -n geocache mod_geocache.la

.c.o:
	$(APXS) -Wc,"$(CFLAGS)" -I../include $(PNG_INC) $(CURL_CFLAGS) $(XML2_CFLAGS) -c $<
	
mod_geocache.o:  mod_geocache.c $(OBJS)
	$(APXS) -Wc,"$(CFLAGS)" -I../include $(JPEG_INC) $(PNG_INC) $(CURL_CFLAGS) $(XML2_CFLAGS) $(XML2_LIBS) $(CURL_LIBS) $(JPEG_LIB) $(PNG_LIB) -c mod_geocache.c $(LOBJS)
	
geocache.fcgi: fastcgi_geocache.c $(OBJS)
	$(CC) -o geocache.fcgi -I../include $(XML2_CFLAGS) -DLINUX=2 -D_FORTIFY_SOURCE=2 -D_GNU_SOURCE -D_REENTRANT -I/usr/include/apr-1.0 -I/usr/include/openssl -I/usr/include/xmltok -pthread $(XML2_LIBS) $(CURL_LIBS) $(JPEG_LIB) $(PNG_LIB) -lapr-1 -lfcgi fastcgi_geocache.c $(OBJS)
	
.libs/mod_geocache.so: mod_geocache.o $(OBJS)


clean:
	rm -f *.o
	rm -f *.so
	rm -f *.slo
	rm -f *.lo
	rm -rf .libs
	rm -f *.la
	rm -f *.sla
	rm -f geocache.fcgi


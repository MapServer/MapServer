include ../Makefile.inc

all: .libs/mod_yatc.so

install: .libs/mod_yatc.so
	$(APXS) -i -a -n yatc mod_yatc.la

.c.o:
	$(APXS) -Wc,"$(CFLAGS)" -I../include $(CAIRO_INC) $(MERGE_ENABLED) $(CAIRO_ENABLED) $(CURL_CFLAGS) $(XML2_CFLAGS) -c $<
	
mod_yatc.o:  mod_yatc.c $(OBJS)
	$(APXS) -Wc,"$(CFLAGS)" -I../include $(CAIRO_INC) $(MERGE_ENABLED) $(CAIRO_ENABLED) $(CURL_CFLAGS) $(XML2_CFLAGS) $(XML2_LIBS) $(CURL_LIBS) $(CAIRO_LIB) -c mod_yatc.c $(LOBJS)
	
.libs/mod_yatc.so: mod_yatc.o $(OBJS)


clean:
	rm -f *.o
	rm -f *.so
	rm -f *.slo
	rm -f *.lo
	rm -rf .libs
	rm -f *.la
	rm -f *.sla


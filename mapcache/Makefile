include ./Makefile.inc

all: .header
	cd lib; $(MAKE) $(MFLAGS)
	cd util; $(MAKE) $(MFLAGS)
	cd cgi; $(MAKE) $(MFLAGS)
	
install: .header install-module install-lib install-util install-cgi

install-util: .header install-lib
	$(INSTALL) -d $(bindir)
	cd util; $(MAKE) $(MFLAGS) install

install-cgi: .header install-lib
	cd cgi; $(MAKE) $(MFLAGS) install

install-lib: .header
	$(INSTALL) -d $(libdir)
	cd lib; $(MAKE) $(MFLAGS) install

module: .header
	cd apache; $(MAKE) $(MFLAGS)

install-module: .header install-lib module
	cd apache; $(MAKE) $(MFLAGS) install

# make clean and rerun if essential files have been modified
.header: configure include/mapcache.h include/util.h include/errors.h Makefile Makefile.inc
	$(MAKE) clean
	@touch .header
	
clean:
	cd lib; $(MAKE) clean
	cd util; $(MAKE) clean
	cd cgi; $(MAKE) clean
	cd apache; $(MAKE) clean

configure: configure.in
	@autoconf
	@echo "configure has been updated/created and should be (re)run before continuing"
	@echo $(shell sh ./reconfigure.sh)
	@exit 1

apache-restart:
	$(APACHECTL) restart

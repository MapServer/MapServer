include ./Makefile.inc

all: .header
	cd src; $(MAKE) $(MFLAGS)
	
install-lib: .header
	cd src; $(MAKE) $(MFLAGS) install-lib

install: .header install-lib
	cd src; $(MAKE) $(MFLAGS) install

install-module: .header install-lib
	cd src; $(MAKE) $(MFLAGS) install-module

# make clean and rerun if essential files have been modified
.header: configure include/mapcache.h include/util.h include/errors.h Makefile Makefile.inc src/Makefile
	$(MAKE) clean
	@touch .header
	
clean:
	cd src; $(MAKE) clean

configure: configure.in
	@autoconf
	@echo "configure has been updated/created and should be (re)run before continuing"
	@echo $(shell sh ./reconfigure.sh)
	@exit 1

apache-restart:
	$(APACHECTL) restart

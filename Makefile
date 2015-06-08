include defs.mk

.PHONY: all gtest rialto_lib rialto_test rialto_tool clean install

all: gtest rialto_lib rialto_test rialto_tool

gtest:
	cd vendor/gtest-1.7.0/ ; \
	cmake -G "Unix Makefiles" . ; \
	make

rialto_lib:
	$(MAKE) -C src all

rialto_test: rialto_lib gtest
	$(MAKE) -C test all

rialto_tool: rialto_lib
	$(MAKE) -C tool all

clean:
	$(MAKE) -C tool clean
	$(MAKE) -C test clean
	$(MAKE) -C src clean

install: all
	mkdir -p $(INSTALL_DIR)/include/ $(INSTALL_DIR)/lib/ $(INSTALL_DIR)/bin/
	cp -f include/rialto/*.hpp $(INSTALL_DIR)/include/
	cp -f src/obj/librialto.a $(INSTALL_DIR)/lib
	cp -f tool/obj/rialto_tool $(INSTALL_DIR)/bin/

#export DYLD_LIBRARY_PATH=$HOME/work/dev/install/pdal/lib/:$DYLD_LIBRARY_PATH

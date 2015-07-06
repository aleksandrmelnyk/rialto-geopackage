include defs.mk

.PHONY: all gtest rialto_lib rialto_test rialto_translate rialto_info clean install test

all: gtest rialto_lib rialto_test rialto_tools rialto_info

gtest:
	cd vendor/gtest-1.7.0/ ; \
	cmake -G "Unix Makefiles" . ; \
	make

rialto_lib:
	$(MAKE) -C src all

rialto_test: rialto_lib gtest
	$(MAKE) -C test all

rialto_tools: rialto_lib
	$(MAKE) -C tool all

rialto_info: rialto_lib
	$(MAKE) -C tool all

clean:
	$(MAKE) -C src clean
	$(MAKE) -C tool clean
	$(MAKE) -C test clean

test:
		./test/obj/rialto_test

install: all
	mkdir -p $(INSTALL_DIR)/include/ $(INSTALL_DIR)/lib/ $(INSTALL_DIR)/bin/
	cp -f include/rialto/*.hpp $(INSTALL_DIR)/include/
	cp -f src/obj/librialto.so $(INSTALL_DIR)/lib
	cp -f tool/obj/rialto_translate $(INSTALL_DIR)/bin/
	cp -f tool/obj/rialto_info $(INSTALL_DIR)/bin/



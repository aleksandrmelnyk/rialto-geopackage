include defs.mk

.PHONY: all gtest library tester app clean

all: gtest library tester app

gtest:
	cd vendor/gtest-1.7.0/ ; \
	cmake -G "Unix Makefiles" . ; \
	make

library:
	$(MAKE) -C src all

tester: library
	$(MAKE) -C test all

app: library
	$(MAKE) -C tool all

clean:
	$(MAKE) -C tool clean
	$(MAKE) -C test clean
	$(MAKE) -C src clean


#export DYLD_LIBRARY_PATH=$HOME/work/dev/install/pdal/lib/:$DYLD_LIBRARY_PATH

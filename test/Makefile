include ../defs.mk

CFLAGS=	-g -std=c++11 -fPIC \
-I../include \
-I$(INSTALL_DIR)/include \
-I../vendor/gtest-1.7.0/include

LDFLAGS=-Wl,-rpath,$(INSTALL_DIR)/lib -L$(INSTALL_DIR)/lib \
-L../vendor/gtest-1.7.0

CC=c++

OBJS=obj/GeoPackageTest.o obj/RialtoReaderTest.o obj/RialtoWriterTest.o obj/RialtoTest.o obj/main.o \
obj/TileMathTest.o

DEPS=RialtoTest.hpp

all: obj/rialto_test

obj/rialto_test: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) -lpdalcpp -lpdal_util -lsqlite3 -lgtest -llaszip -lrialto -lpthread

obj/%.o: %.cpp
	@mkdir -p ./obj
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJS): $(DEPS)

install: obj/tester
	echo "Nothing to install in test"

test: 
	obj/tester
clean:
	rm -f obj/*

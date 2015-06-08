include ../defs.mk

CFLAGS=	-g -std=c++11 \
-I../include \
-I$(PDAL_HOME)/include \
-I../vendor/gtest-1.7.0/include

LDFLAGS=-L$(PDAL_HOME)/lib \
-L../vendor/gtest-1.7.0 \
-L../src/obj

CC=c++

OBJS=obj/GeoPackageTest.o obj/RialtoReaderTest.o obj/RialtoWriterTest.o obj/RialtoTest.o obj/main.o \
obj/TileMathTest.o

DEPS=RialtoTest.hpp

all: obj/tester

obj/tester: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) -lrialto -lpdalcpp -lpdal_util -lsqlite3 -lgtest -llaszip -lpthread

obj/%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJS): $(DEPS)

clean:
	rm -f obj/*

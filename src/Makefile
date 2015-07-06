include ../defs.mk

CFLAGS=	-g -std=c++11 -fPIC -O3 \
-I ../include \
-I $(INSTALL_DIR)/include

LDFLAGS=-shared -L$(INSTALL_DIR)/lib
CC=c++

OBJS=obj/Event.o obj/GeoPackage.o obj/GeoPackageReader.o obj/RialtoWriter.o \
obj/GeoPackageCommon.o obj/GeoPackageWriter.o obj/WritableTileCommon.o \
obj/GeoPackageManager.o obj/RialtoReader.o 

DEPS=\
../include/rialto/Event.hpp \
../include/rialto/GeoPackage.hpp \
../include/rialto/GeoPackageCommon.hpp \
../include/rialto/GeoPackageManager.hpp \
../include/rialto/GeoPackageReader.hpp \
../include/rialto/GeoPackageWriter.hpp \
../include/rialto/RialtoReader.hpp \
../include/rialto/RialtoWriter.hpp \
./SQLiteCommon.hpp \
./TileMath.hpp \
./WritableTileCommon.hpp

.PHONY: all install clean

all: obj/librialto.so

obj/librialto.so: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ -lboost_filesystem -lsqlite3 -lpdalcpp -lpdal_util

obj/%.o: %.cpp
	@mkdir -p ./obj
	$(CC) $(CFLAGS) -c -o $@  $<

$(OBJS): $(DEPS)

install: obj/librialto.so
	$(MAKE) all
	cp $< $(INSTALL_DIR)/lib

clean:
	rm -f obj/*

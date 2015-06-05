include ../defs.mk

CFLAGS=	-g -std=c++11 -ferror-limit=3 \
-I ../include \
-I $(PDAL_HOME)/include

LDFLAGS=-L$(PDAL_HOME)/lib \
-lpdalcpp -lpdal_util

CC=c++


OBJS=obj/GeoPackage.o obj/GeoPackageReader.o obj/RialtoWriter.o \
obj/GeoPackageCommon.o obj/GeoPackageWriter.o obj/ViewTileCommon.o \
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
../include/rialto/SQLiteCommon.hpp \
../include/rialto/TileMath.hpp \
../include/rialto/ViewTileCommon.hpp

all: obj/librialto.a

obj/librialto.a: $(OBJS)
	ar rcs $@ $^

obj/%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJS): $(DEPS)

clean:
	rm -f obj/*
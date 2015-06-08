include ../defs.mk


CFLAGS=	-g -std=c++11 \
-I../include \
-I$(PDAL_HOME)/include

LDFLAGS=-L$(PDAL_HOME)/lib \
-L../src/obj

CC=c++

OBJS=obj/Tool.o obj/main.o

DEPS=Tool.hpp

all: obj/rialto_tool

obj/rialto_tool: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) -lpdalcpp -lpdal_util -lrialto -lsqlite3 -llaszip

obj/%.o: %.cpp
	@mkdir -p ./obj
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJS): $(DEPS)

clean:
	rm -f obj/*

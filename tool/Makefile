include ../defs.mk


CFLAGS=	-g -std=c++11 \
-I../include \
-I$(INSTALL_DIR)/include

LDFLAGS=-Wl,-rpath,$(INSTALL_DIR)/lib -L$(INSTALL_DIR)/lib

CC=c++

OBJS=obj/Tool.o obj/main.o

DEPS=Tool.hpp

all: obj/rialto_tool

obj/rialto_tool: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) -lpdalcpp -lpdal_util -lrialto -lsqlite3 -llaszip -lpthread

obj/%.o: %.cpp
	@mkdir -p ./obj
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJS): $(DEPS)

install: obj/tool
	$(MAKE) all
	cp $< $(INSTALL_DIR)/bin

clean:
	rm -f obj/*

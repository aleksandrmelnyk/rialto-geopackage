include ../defs.mk


CFLAGS=	-g -std=c++11 -O3 \
-I../include \
-I$(INSTALL_DIR)/include

LDFLAGS=-Wl,-rpath,$(INSTALL_DIR)/lib -L$(INSTALL_DIR)/lib

CC=c++

OBJS=obj/Tool.o obj/InfoTool.o obj/TranslateTool.o

DEPS=Tool.hpp

all: obj/rialto_info obj/rialto_translate

obj/rialto_info: $(OBJS) obj/rialto_info.o
	$(CC) -o $@ $^ $(LDFLAGS) -lpdalcpp -lpdal_util -lrialto -lsqlite3 -llaszip -lpthread

obj/rialto_translate: $(OBJS) obj/rialto_translate.o
		$(CC) -o $@ $^ $(LDFLAGS) -lpdalcpp -lpdal_util -lrialto -lsqlite3 -llaszip -lpthread

obj/%.o: %.cpp
	@mkdir -p ./obj
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJS): $(DEPS)

install: obj/rialto_info obj/rialto_translate
	$(MAKE) all
	cp $< $(INSTALL_DIR)/bin

clean:
	rm -f obj/*

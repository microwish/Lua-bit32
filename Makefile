NAME = lua-bit32
VERSION = 0.1
DIST := $(NAME)-$(VERSION)

CC = gcc
RM = rm -rf

CFLAGS = -Wall -g -fPIC -I/home/microwish/lua/include
#CFLAGS = -Wall -g -O2 -fPIC -I/home/microwish/lua/include
LFLAGS = -shared -L/home/microwish/lua/lib -llua
INSTALL_PATH = /home/microwish/lua-bit32/lib

all: bit32.so

bit32.so: bit32.o
  $(CC) -o $@ $< $(LFLAGS)

bit32.o: lbit32lib.c
	$(CC) -o $@ $(CFLAGS) -c $<

install: bit32.so
	install -D -s $< $(INSTALL_PATH)/$<

clean:
	$(RM) *.so *.o

dist:
	if [ -d $(DIST) ]; then $(RM) $(DIST); fi
	mkdir -p $(DIST)
	cp *.c Makefile $(DIST)/
	tar czvf $(DIST).tar.gz $(DIST)
	$(RM) $(DIST)

.PHONY: all clean dist

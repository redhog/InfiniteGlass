PACKAGES=librsvg-2.0 libxml-2.0

CC=gcc
CFLAGS=$(shell pkg-config --cflags $(PACKAGES)) -O0 -Wall
LIBS:=$(shell pkg-config --libs $(PACKAGES))

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test: test.o svg_use_manager.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

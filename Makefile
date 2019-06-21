CC=gcc
CFLAGS=-ggdb

LIBS=-lX11 -lGL -lGLU -lGLEW -lXrender -lXcomposite -lXtst -lXdamage -lXext -lXfixes -lXrandr -lSOIL $(shell imlib2-config --libs)

all: run

bin/%: bin

bin:
	mkdir -p bin

bin/%.o : %.c *.h
	$(CC) $(CFLAGS) -c $< -o $@

bin/wm: $(patsubst %.c,bin/%.o, $(wildcard *.c))
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)
run: bin/wm
	#xinit ./xinitrc -- "$$(whereis -b Xephyr | cut -f2 -d' ')" :100 -ac -screen 800x600 -host-cursor
	xinit ./xinitrc -- "$$(whereis -b Xephyr | cut -f2 -d' ')" :100 -ac -screen 800x600 -host-cursor &
	gdb -ex "target remote localhost:2048" -ex "continue" ./bin/wm

clean:
	rm -f bin/wm bin/*.o *~

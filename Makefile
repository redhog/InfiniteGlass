CC=gcc
CFLAGS=-ggdb

all: run

bin/%: bin

bin:
	mkdir -p bin

bin/%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

bin/wm: bin/wm.o bin/xapi.o bin/glapi.o bin/shader.o bin/space.o bin/input.o bin/xevent.o
	$(CC) $(CFLAGS) -o $@ $^ -lX11 -lGL -lGLU -lGLEW -lXrender -lXcomposite -lXtst -lXdamage -lXext -lXfixes -lXrandr -lSOIL

run: bin/wm
	xinit ./xinitrc -- "$$(whereis -b Xephyr | cut -f2 -d' ')" :100 -ac -screen 800x600 -host-cursor

clean:
	rm -f bin/wm bin/*.o *~

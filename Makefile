CC=gcc
CFLAGS=-ggdb

all: run

bin/%: bin

bin:
	mkdir -p bin

bin/%.o : %.c *.h
	$(CC) $(CFLAGS) -c $< -o $@

bin/wm: bin/wm.o bin/xapi.o bin/glapi.o bin/shader.o bin/item.o bin/item_window.o bin/input.o bin/xevent.o bin/screen.o actions.o
	$(CC) $(CFLAGS) -o $@ $^ -lX11 -lGL -lGLU -lGLEW -lXrender -lXcomposite -lXtst -lXdamage -lXext -lXfixes -lXrandr -lSOIL

run: bin/wm
	#xinit ./xinitrc -- "$$(whereis -b Xephyr | cut -f2 -d' ')" :100 -ac -screen 800x600 -host-cursor
	xinit ./xinitrc -- "$$(whereis -b Xephyr | cut -f2 -d' ')" :100 -ac -screen 800x600 -host-cursor &
	gdb -ex "target remote localhost:2048" -ex "continue" ./bin/wm

clean:
	rm -f bin/wm bin/*.o *~

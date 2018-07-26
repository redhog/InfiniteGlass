CC=gcc
CFLAGS=-ggdb


all: run

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

wm: wm.o xapi.o glapi.o shader.o space.o input.o
	$(CC) $(CFLAGS) -o $@ $^ -lX11 -lGL -lGLU -lGLEW -lXrender -lXcomposite -lXtst -lXdamage -lXext -lSOIL

run: wm
	xinit ./xinitrc -- "$$(whereis -b Xephyr | cut -f2 -d' ')" :100 -ac -screen 800x600 -host-cursor

clean:
	rm -f wm *.o *~

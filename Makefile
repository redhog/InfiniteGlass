CC=gcc
CFLAGS=


all: run

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

wm: wm.o xapi.o glapi.o shader.o space.o
	gcc -o $@ $^ -lX11 -lGL -lGLU -lGLEW -lXrender -lXcomposite -lXtst -lXdamage -lXext -lSOIL

run: wm
	xinit ./xinitrc -- "$$(whereis -b Xephyr | cut -f2 -d' ')" :100 -ac -screen 800x600 -host-cursor

clean:
	rm -f wm *.o *~

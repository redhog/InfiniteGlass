CC=gcc
CFLAGS=


all: run

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

wm: wm.o
	gcc -o $@ $< -lX11 -lGL -lGLU -lXrender -l Xcomposite

bindtex: bindtex.o
	gcc -o $@ $< -lX11 -lGL -lGLU -lXrender -l Xcomposite

run: wm
	xinit ./xinitrc -- "$$(whereis -b Xephyr | cut -f2 -d' ')" :100 -ac -screen 800x600 -host-cursor

clean:
	rm -f wm bindtex *.o

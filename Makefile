CC=gcc
CFLAGS=


all: run

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

testgl: testgl.o shader.o
	gcc -o $@ $^ -lglut -lGLEW -lX11 -lGL -lGLU -lXrender -lXcomposite -lXtst

wm: wm.o xapi.o glapi.o shader.o
	gcc -o $@ $^ -lX11 -lGL -lGLU -lGLEW -lXrender -lXcomposite -lXtst -lSOIL

run: wm
	xinit ./xinitrc -- "$$(whereis -b Xephyr | cut -f2 -d' ')" :100 -ac -screen 800x600 -host-cursor

clean:
	rm -f wm *.o *~

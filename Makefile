all: run

wm: wm.c
	gcc -o wm wm.c -lX11 -lGL -lGLU -lXrender -l Xcomposite

bindtex: bindtex.c
	gcc -o bindtex bindtex.c -lX11 -lGL -lGLU -lXrender -l Xcomposite

run: wm
	xinit ./xinitrc -- "$$(whereis -b Xephyr | cut -f2 -d' ')" :100 -ac -screen 800x600 -host-cursor

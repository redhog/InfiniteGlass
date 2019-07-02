CC=gcc
CFLAGS=-ggdb $(shell pkg-config --cflags librsvg-2.0 pangocairo cairo)

LIBS=-lX11 -lGL -lGLU -lGLEW -lXrender -lXcomposite -lXtst -lXdamage -lXext -lXfixes -lXrandr -lSOIL $(shell pkg-config --libs librsvg-2.0 pangocairo cairo)

all: run

bin/%: bin

bin:
	mkdir -p bin

fontawesome-free-5.9.0-desktop.zip:
	wget https://use.fontawesome.com/releases/v5.9.0/fontawesome-free-5.9.0-desktop.zip

fontawesome-free-5.9.0-desktop: fontawesome-free-5.9.0-desktop.zip
	unzip fontawesome-free-5.9.0-desktop.zip
	#mv fontawesome-free-5.9.0-desktop fontawesome

fontawesome/%.otf: fontawesome-free-5.9.0-desktop
	mkdir -p fontawesome
	(cd fontawesome-free-5.9.0-desktop/otfs; ls *.otf; ) | while read name; do cp "fontawesome-free-5.9.0-desktop/otfs/$$name" "fontawesome/$$(echo "$$name" | tr " " -)"; done

%.ttf: %.otf
	fontforge -script otf2ttf.sh $<

bin/%.o : %.c *.h
	$(CC) $(CFLAGS) -c $< -o $@

bin/wm: $(patsubst %.c,bin/%.o, $(wildcard *.c))
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

run: bin/wm fontawesome/Font-Awesome-5-Free-Regular-400.ttf
	#xinit ./xinitrc -- "$$(whereis -b Xephyr | cut -f2 -d' ')" :100 -ac -screen 800x600 -host-cursor
	xinit ./xinitrc -- "$$(whereis -b Xephyr | cut -f2 -d' ')" :100 -ac -screen 1024x768 -host-cursor &
	gdb -ex "target remote localhost:2048" -ex "continue" ./bin/wm

clean:
	rm -f bin/wm bin/*.o *~

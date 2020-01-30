CC=gcc
PACKAGES=librsvg-2.0 pangocairo cairo xi x11 gl glu glew xrender xcomposite xtst xdamage xext xfixes xrandr
CFLAGS=-rdynamic -g -ggdb $(shell pkg-config --cflags $(PACKAGES)) -O0 -Wall $(EXTRA_CFLAGS)
LIBS:=-lbacktrace -ldl $(shell pkg-config --libs $(PACKAGES))

ifeq ($(GLASS_DMALLOC),)
else
  LIBS:=$(LIBS) -ldmalloc
endif

BUILD=../build

all: $(BUILD)/glass-renderer

$(BUILD)/%.o: %.c *.h
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/glass-renderer: $(patsubst %.c,$(BUILD)/%.o, $(wildcard *.c))
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $(patsubst %.c,$(BUILD)/%.o, $(wildcard *.c)) $(LIBS)


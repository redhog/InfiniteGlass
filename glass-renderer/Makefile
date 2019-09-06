CC=gcc
PACKAGES=librsvg-2.0 pangocairo cairo xi x11 gl glu glew xrender xcomposite xtst xdamage xext xfixes xrandr
CFLAGS=-ggdb $(shell pkg-config --cflags $(PACKAGES))
LIBS:=-lSOIL $(shell pkg-config --libs $(PACKAGES))

ifeq ($(GLASS_DMALLOC),)
else
  LIBS:=$(LIBS) -ldmalloc
endif

BUILD=../build

all: $(BUILD)/glass-renderer

$(BUILD)/%.o: %.c *.h
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/glass-renderer: *.glsl
$(BUILD)/glass-renderer: $(patsubst %.c,$(BUILD)/%.o, $(wildcard *.c))
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $(patsubst %.c,$(BUILD)/%.o, $(wildcard *.c)) $(LIBS)
	cp *.glsl $(BUILD)/

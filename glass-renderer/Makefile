CC=gcc
PACKAGES=librsvg-2.0 pangocairo cairo xi x11 x11-xcb xcb xcb-icccm xcb-damage gl glu glew xrender xcomposite xtst xdamage xext xfixes xrandr

ifeq ($(GLASS_PROFILE),1)
  PROFILE_CFLAGS:=-pg
else
  PROFILE_CFLAGS:=
endif

CFLAGS=-rdynamic -g -ggdb $(shell pkg-config --cflags $(PACKAGES)) -O0 -Wall $(PROFILE_CFLAGS) $(EXTRA_CFLAGS)
LIBS:=-ldl $(shell pkg-config --libs $(PACKAGES))

ifeq ($(GLASS_DMALLOC),)
else
  LIBS:=$(LIBS) -ldmalloc
endif

ifndef GLASS_USE_BACKTRACE
else
  LIBS:=$(LIBS) -lbacktrace
  CFLAGS:=$(CFLAGS) -DUSE_BACKTRACE
endif

BUILD=../build

all: $(BUILD)/glass-renderer

$(BUILD)/%.o: %.c *.h
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/glass-renderer: $(patsubst %.c,$(BUILD)/%.o, $(wildcard *.c))
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $(patsubst %.c,$(BUILD)/%.o, $(wildcard *.c)) $(LIBS)


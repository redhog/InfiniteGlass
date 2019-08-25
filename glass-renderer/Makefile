CC=gcc
CFLAGS=-ggdb $(shell pkg-config --cflags librsvg-2.0 pangocairo cairo)

LIBS=-lX11 -lGL -lGLU -lGLEW -lXrender -lXcomposite -lXtst -lXdamage -lXext -lXfixes -lXrandr -lSOIL $(shell pkg-config --libs librsvg-2.0 pangocairo cairo)

BUILD=../build

all: $(BUILD)/glass-renderer

$(BUILD)/%.o: %.c *.h
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/glass-renderer: $(patsubst %.c,$(BUILD)/%.o, $(wildcard *.c))
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

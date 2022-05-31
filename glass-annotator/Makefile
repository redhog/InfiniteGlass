CC=gcc
CFLAGS=-Wall -fPIC -shared
LIBS:=-ldl

BUILD=../build

all: $(BUILD)/glass-annotator

$(BUILD)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/glass-annotator: $(patsubst %.c,$(BUILD)/%.o, $(wildcard *.c))
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $(patsubst %.c,$(BUILD)/%.o, $(wildcard *.c)) $(LIBS)


all: run

BUILD=build

SUBDIRS := $(wildcard */.)
BINARIES := $(patsubst %/.,$(BUILD)/%,$(SUBDIRS))
PYTHONAPPS := $(patsubst %/.,$(BUILD)/env/bin/%,$(SUBDIRS))

fontawesome-free-5.9.0-desktop.zip:
	wget https://use.fontawesome.com/releases/v5.9.0/fontawesome-free-5.9.0-desktop.zip

fontawesome-free-5.9.0-desktop: fontawesome-free-5.9.0-desktop.zip
	unzip fontawesome-free-5.9.0-desktop.zip

fontawesome/%.otf: fontawesome-free-5.9.0-desktop
	mkdir -p fontawesome
	(cd fontawesome-free-5.9.0-desktop/otfs; ls *.otf; ) | while read name; do cp "fontawesome-free-5.9.0-desktop/otfs/$$name" "fontawesome/$$(echo "$$name" | tr " " -)"; done

%.ttf: %.otf
	fontforge -script otf2ttf.sh $<

$(BINARIES): $(BUILD)

$(BUILD):
	mkdir -p $(BUILD)

.PHONY: $(BINARIES)
$(BINARIES):
	$(MAKE) -C $(notdir $@)

$(BUILD)/env:
	virtualenv --python=python3 $@
	. $@/bin/activate; cd pyig; pip install numpy
	. $@/bin/activate; cd pyig; python setup.py develop

$(PYTHONAPPS): $(BUILD)/env
	. $(BUILD)/env/bin/activate; cd $(notdir $@); python setup.py develop

run: $(BUILD)/env $(BUILD)/wm $(BUILD)/env/bin/glass-animator $(BUILD)/env/bin/glass-widgets $(BUILD)/env/bin/glass-input $(BUILD)/env/bin/glass-ghosts fontawesome/Font-Awesome-5-Free-Regular-400.ttf
	xinit ./xinitrc -- "$$(whereis -b Xephyr | cut -f2 -d' ')" :100 -ac -screen 1024x768 -host-cursor &
	gdb -ex "target remote localhost:2048" -ex "continue" ./$(BUILD)/wm

clean:
	rm -rf $(BUILD)

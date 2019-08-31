all: run

BUILD=build
PREFIX=/usr/local

BINARIES_SUBDIRS := $(patsubst %/Makefile,%,$(wildcard */Makefile))
BINARIES := $(patsubst %,$(BUILD)/%,$(BINARIES_SUBDIRS))

PYTHONAPPS_SUBDIRS := $(patsubst %/setup.py,%,$(wildcard */setup.py))
PYTHONAPPS := $(patsubst %,$(BUILD)/env/bin/%,$(PYTHONAPPS_SUBDIRS))

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

run: $(BINARIES) $(PYTHONAPPS)
	xinit ./xinitrc -- "$$(whereis -b Xephyr | cut -f2 -d' ')" :100 -ac -screen 1024x768 -host-cursor &
	gdb -ex "target remote localhost:2048" -ex "continue" ./$(BUILD)/wm

run-xorg: $(BINARIES) $(PYTHONAPPS)
	xinit ./xinitrc -- "$$(whereis -b Xorg | cut -f2 -d' ')" :100 -ac &
	gdb -ex "target remote localhost:2048" -ex "continue" ./$(BUILD)/wm

install: $(BINARIES) $(patsubst %,install-%,$(PYTHONAPPS_SUBDIRS))
	cp $(BUILD)/glass-renderer $(PREFIX)/bin/glass-renderer
	mkdir -p $(PREFIX)/share/glass
	cp $(BUILD)/*.glsl $(PREFIX)/share/glass
	cp glass-startup.sh $(PREFIX)/bin/glass-startup.sh
	cp glass.desktop /usr/share/xsessions/glass.desktop

$(patsubst %,install-%,$(PYTHONAPPS_SUBDIRS)):
	cd $(patsubst install-%,%,$@); python3 setup.py install

clean:
	rm -rf $(BUILD)

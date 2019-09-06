all: run

XSERVER=Xephyr
GLASS_DEBUG=gdb

ifeq ($(XSERVEROPTS),)
  ifeq ($(XSERVER),Xephyr)
    XSERVEROPTS= :100 -ac -screen 1024x768 -host-cursor
  else
    XSERVEROPTS= :100 -ac
  endif
endif

XSERVERPATH=$(shell whereis -b $(XSERVER) | cut -f2 -d' ')

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
	$(MAKE) GLASS_DMALLOC="$(GLASS_DMALLOC)" -C $(notdir $@)

$(BUILD)/env:
	virtualenv --python=python3 $@
	. $@/bin/activate; cd pyig; pip install numpy
	. $@/bin/activate; cd pyig; python setup.py develop

$(PYTHONAPPS): $(BUILD)/env
	. $(BUILD)/env/bin/activate; cd $(notdir $@); python setup.py develop

run: $(BINARIES) $(PYTHONAPPS)
	GLASS_DEBUG="$(GLASS_DEBUG)" BUILD="$(BUILD)" XSERVERPATH="$(XSERVERPATH)" XSERVEROPTS="$(XSERVEROPTS)" ./xstartup.sh

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

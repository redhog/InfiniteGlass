.PHONY: default
default: run

XSERVER=Xephyr
GLASS_DEBUGGER=

ifeq ($(XSERVEROPTS),)
  ifeq ($(XSERVER),Xephyr)
    XSERVEROPTS= :100 -ac -screen 1280x768 -host-cursor
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
	. $@/bin/activate; cd glass-lib; python setup.py develop

$(PYTHONAPPS): $(BUILD)/env
	. $(BUILD)/env/bin/activate; cd $(notdir $@); python setup.py develop

.PHONY: all run install devinstall uninstall install-binaries uninstall-binaries
all: $(BINARIES) $(PYTHONAPPS)

run: all
	GLASS_DEBUGGER="$(GLASS_DEBUGGER)" BUILD="$(BUILD)" XSERVERPATH="$(XSERVERPATH)" XSERVEROPTS="$(XSERVEROPTS)" ./xstartup.sh

install: install-binaries $(patsubst %,install-%,$(PYTHONAPPS_SUBDIRS))

devinstall: install-binaries $(patsubst %,devinstall-%,$(PYTHONAPPS_SUBDIRS))

uninstall: uninstall-binaries $(patsubst %,uninstall-%,$(PYTHONAPPS_SUBDIRS))

install-binaries: $(BINARIES)
	cp $(BUILD)/glass-renderer $(PREFIX)/bin/glass-renderer
	mkdir -p $(PREFIX)/share/glass
	cp glass-startup.sh $(PREFIX)/bin/glass-startup.sh
	cp glass.desktop /usr/share/xsessions/glass.desktop

uninstall-binaries:
	rm $(PREFIX)/bin/glass-renderer
	rm -rf $(PREFIX)/share/glass
	rm $(PREFIX)/bin/glass-startup.sh
	rm /usr/share/xsessions/glass.desktop

$(patsubst %,install-%,$(PYTHONAPPS_SUBDIRS)):
	cd $(patsubst install-%,%,$@); python3 setup.py install

$(patsubst %,devinstall-%,$(PYTHONAPPS_SUBDIRS)):
	cd $(patsubst devinstall-%,%,$@); python3 setup.py develop

$(patsubst %,uninstall-%,$(PYTHONAPPS_SUBDIRS)):
	pip3 uninstall $(patsubst uninstall-%,%,$@)

clean:
	rm -rf $(BUILD)

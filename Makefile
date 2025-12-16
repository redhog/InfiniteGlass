.PHONY: default
default: run

XSERVER=Xephyr
GLASS_DEBUGGER=
PYTHON=python3

ifeq ($(XSERVEROPTS),)
  ifeq ($(XSERVER),Xephyr)
    XSERVEROPTS= :100 -ac -screen 1280x768x24 -host-cursor -extension MIT-SHM -nolisten tcp
  else
    XSERVEROPTS= :100 -ac
  endif
endif

XSERVERPATH=$(shell whereis -b $(XSERVER) | cut -f2 -d' ')

BUILD=build
BINDIR=build/env/bin
PREFIX=/usr/local

PYTHONAPPS_SUBDIRS := $(patsubst %/setup.py,%,$(wildcard */setup.py))
PYTHONAPPS := $(patsubst %,$(BUILD)/env/bin/%,$(PYTHONAPPS_SUBDIRS))

SCRIPTS := $(patsubst scripts/%,$(BINDIR)/%,$(wildcard scripts/*))

$(BUILD)/env/bin/activate: 
	virtualenv --python=$(PYTHON) $(BUILD)/env
	. $(BUILD)/env/bin/activate; pip install setuptools
	. $(BUILD)/env/bin/activate; cd glass-lib; pip install -e .

$(SCRIPTS): $(BUILD)/env/bin/activate
	cp scripts/* $(BINDIR)

$(PYTHONAPPS): $(BUILD)/env/bin/activate
	. $(BUILD)/env/bin/activate; cd $(notdir $@); pip install -e .

.PHONY: all run run-in-docker install devinstall uninstall install-binaries uninstall-binaries
all: $(SCRIPTS) $(PYTHONAPPS)

run: all
	GLASS_DEBUGGER="$(GLASS_DEBUGGER)" BUILD="$(BUILD)" XSERVERPATH="$(XSERVERPATH)" XSERVEROPTS="$(XSERVEROPTS)" scripts/xstartup.sh

run-in-docker:
	scripts/run-in-docker.sh

install: install-binaries $(patsubst %,install-%,$(PYTHONAPPS_SUBDIRS))

devinstall: install-binaries $(patsubst %,devinstall-%,$(PYTHONAPPS_SUBDIRS))

uninstall: uninstall-binaries $(patsubst %,uninstall-%,$(PYTHONAPPS_SUBDIRS))

install-binaries:
	mkdir -p $(PREFIX)/share/glass
	mkdir -p $(PREFIX)/bin
	mkdir -p /usr/share/xsessions
	mkdir -p /usr/share/applications
	mkdir -p /etc/emacs/site-start.d
	cp scripts/glass-startup.sh $(PREFIX)/bin/glass-startup.sh
	cp scripts/glass.desktop /usr/share/xsessions/glass.desktop
	cp scripts/glass-chromium-browser.desktop /usr/share/applications/glass-chromium-browser.desktop
	cp scripts/glass-emacs-xsession.el /etc/emacs/site-start.d/glass-emacs-xsession.el

uninstall-binaries:
	rm -rf $(PREFIX)/share/glass
	rm $(PREFIX)/bin/glass-startup.sh
	rm /usr/share/xsessions/glass.desktop
	rm /usr/share/applications/glass-chromium-browser.desktop
	rm /etc/emacs/site-start.d/glass-emacs-xsession.el

$(patsubst %,install-%,$(PYTHONAPPS_SUBDIRS)):
	cd $(patsubst install-%,%,$@); pip3 install .

$(patsubst %,devinstall-%,$(PYTHONAPPS_SUBDIRS)):
	cd $(patsubst devinstall-%,%,$@); pip3 install -e .

$(patsubst %,uninstall-%,$(PYTHONAPPS_SUBDIRS)):
	pip3 uninstall $(patsubst uninstall-%,%,$@)

clean: clean-build clean-python clean-docker

clean-build:
	rm -rf $(BUILD)

clean-python:
	rm -rf $(shell find . -name __pycache__) $(shell find . -name .eggs) $(shell find . -name dist) $(shell find . -name build | grep /glass)

clean-docker:
	scripts/run-in-docker.sh clean

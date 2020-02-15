import InfiniteGlass
import pkg_resources
import json
import numpy
import Xlib.X
import sys
import re

class ThemeBase(object):
    def __init__(self, display, **kw):
        self.display = display
        for name, value in kw.items():
            setattr(self, name, value)
        
    def load_shader(self, name):
        if name.startswith("resource://"):
            pkg, name = name.split("://")[1].split("/", 1)
            with pkg_resources.resource_stream(pkg, name) as f:
                src = f.read()
        else:
            with open(name) as f:
                src = f.read()
        includes = re.findall(rb'^#include  *"(.*)"', src, re.MULTILINE)
        for name in includes:
            src = re.sub(rb'#include  *"%s"' % (name,), self.load_shader(name.decode("utf-8")), src, re.MULTILINE)
        return src

    def setup_shaders(self, path, shaders = ("DEFAULT", "ROOT", "SPLASH", "SPLASH_BACKGROUND"), parts = ("GEOMETRY", "VERTEX", "FRAGMENT")):
        self.display.root["IG_SHADER"] = "IG_SHADER_ROOT"
        for SHADER in shaders:
            shader = SHADER.lower()
            for PART in parts:
                part = PART.lower()
                self.display.root["IG_SHADER_%s_%s" % (SHADER, PART)] = self.load_shader("%s/%s/%s.glsl" % (path, shader, part))
        self.display.root["IG_SHADERS"] = ["IG_SHADER_%s" % shader for shader in shaders]

    def setup_views(self, views=["IG_VIEW_ROOT", "IG_VIEW_DESKTOP", "IG_VIEW_OVERLAY", "IG_VIEW_MENU"]):
        self.display.root["IG_VIEW_SPLASH_LAYER"] = "IG_LAYER_SPLASH"
        self.display.root["IG_VIEW_SPLASH_VIEW"] = [0.0, 0.0, 1.0, 0.0]
        self.display.root["IG_VIEW_SPLASH_BACKGROUND_LAYER"] = "IG_LAYER_SPLASH_BACKGROUND"
        self.display.root["IG_VIEW_SPLASH_BACKGROUND_VIEW"] = [0.0, 0.0, 1.0, 0.0]

        self.display.root["IG_VIEW_MENU_LAYER"] = "IG_LAYER_MENU"
        self.display.root["IG_VIEW_MENU_VIEW"] = [0.0, 0.0, 1.0, 0.0]
        self.display.root["IG_VIEW_OVERLAY_LAYER"] = "IG_LAYER_OVERLAY"
        self.display.root["IG_VIEW_OVERLAY_VIEW"] = [0.0, 0.0, 1.0, 0.0]
        self.display.root["IG_VIEW_DESKTOP_LAYER"] = "IG_LAYER_DESKTOP"
        self.display.root["IG_VIEW_DESKTOP_VIEW"] = [0.0, 0.0, 1.0, 0.0]

        self.display.root["IG_VIEW_ROOT_LAYER"] = "IG_LAYER_ROOT"
        self.display.root["IG_VIEW_ROOT_VIEW"] = [0.0, 0.0, 1.0, 0.0]

        self.display.root["IG_VIEWS"] = views

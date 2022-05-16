import InfiniteGlass
import pkg_resources
import json
import numpy
import Xlib.X
import sys
import re
import os.path

class ThemeBase(object):
    def __init__(self, display, **kw):
        self.display = display
        for name, value in kw.items():
            setattr(self, name, value)
        self.setup_shaders()
        self.setup_properties()

    shader_path = None
    shaders = ("DEFAULT", "DECORATION", "ROOT", "SPLASH", "SPLASH_BACKGROUND")
    shader_parts = ("GEOMETRY", "VERTEX", "FRAGMENT")

    root_IG_SHADER = "IG_SHADER_ROOT"
    
    root_IG_VIEW_SPLASH_LAYER = "IG_LAYER_SPLASH"
    root_IG_VIEW_SPLASH_VIEW = [0.0, 0.0, 1.0, 0.0]
    root_IG_VIEW_SPLASH_BACKGROUND_LAYER = "IG_LAYER_SPLASH_BACKGROUND"
    root_IG_VIEW_SPLASH_BACKGROUND_VIEW = [0.0, 0.0, 1.0, 0.0]

    root_IG_VIEW_MENU_LAYER = "IG_LAYER_MENU"
    root_IG_VIEW_MENU_VIEW = [0.0, 0.0, 1.0, 0.0]
    root_IG_VIEW_OVERLAY_LAYER = "IG_LAYER_OVERLAY"
    root_IG_VIEW_OVERLAY_VIEW = [0.0, 0.0, 1.0, 0.0]
    root_IG_VIEW_DESKTOP_LAYER = ["IG_LAYER_ISLAND", "IG_LAYER_DESKTOP"]
    root_IG_VIEW_DESKTOP_VIEW = [0.0, 0.0, 1.0, 0.0]

    root_IG_VIEW_ROOT_LAYER = "IG_LAYER_ROOT"
    root_IG_VIEW_ROOT_VIEW = [0.0, 0.0, 1.0, 0.0]

    root_IG_VIEWS = ["IG_VIEW_ROOT", "IG_VIEW_DESKTOP", "IG_VIEW_OVERLAY", "IG_VIEW_MENU"]

    
    def load_shader(self, name):
        defines = ''.join(
            "#define %s %s\n" %(name[len("define_"):], getattr(self, name))
            for name in dir(self)
            if name.startswith("define_")).encode("utf-8")
        preamble = ""
        src = self._load_shader(name)
        if b"#version" in src:
            vi = src.index(b"#version")
            pi = vi + src[vi:].index(b"\n") + 1
            preamble = src[:pi]
            src = src[pi:]
        return preamble + defines + src
        
    def _load_shader(self, name):
        if name.startswith("resource://"):
            pkg, name = name.split("://")[1].split("/", 1)
            with pkg_resources.resource_stream(pkg, name) as f:
                src = f.read()
        else:
            with open(name) as f:
                src = f.read()
        includes = re.findall(rb'^#include  *"(.*)"', src, re.MULTILINE)
        for name in includes:
            src = re.sub(rb'#include  *"%s"' % (name,), self._load_shader(name.decode("utf-8")), src, re.MULTILINE)
        return src

    def get_shader(self, SHADER, PART):
        part_name = "shader_%s_%s" % (SHADER, PART)
        shader_name = "shader_%s" % (SHADER,)
        if hasattr(self, part_name):
            part = getattr(self, part_name)
        elif hasattr(self, shader_name):
            part = "%s/%s" % (getattr(self, shader_name), PART.lower())
        else:
            part = "%s/%s" % (SHADER.lower(), PART.lower())
        if "." not in part.split("/")[-1]:
            part = "%s.glsl" % (part,)
        if "://" not in part and not part.startswith("./") and not part.startswith("../") and not part.startswith("~"):
            part = "%s/%s" % (self.shader_path, part)
        part = os.path.expanduser(part)            
        return part
    
    def setup_shaders(self):
        for SHADER in self.shaders:
            for PART in self.shader_parts:
                self.display.root["IG_SHADER_%s_%s" % (SHADER, PART)] = self.load_shader(self.get_shader(SHADER, PART))
        self.display.root["IG_SHADERS"] = ["IG_SHADER_%s" % shader for shader in self.shaders]

    def setup_properties(self):
        for name in dir(self):
            if name.startswith("root_"):
                self.display.root[name[len("root_"):]] = getattr(self, name)

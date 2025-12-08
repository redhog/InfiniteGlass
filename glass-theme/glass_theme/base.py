import InfiniteGlass
import re
from .utils import instantiate_config, read_file

class ThemeBase(object):
    def __init__(self, display, **kw):
        self.display = display
        
        recurse = set([name for name, value in kw.items()
                       if isinstance(value, dict)
                       ]).union(
                           [name for name in dir(self)
                            if (not name.startswith("__")
                                and isinstance(getattr(self, name), type)
                                and issubclass(getattr(self, name), ThemeBase))])
        for name in recurse:
            print("Recursing", self, name, getattr(self, name, None))
            setattr(
                self,
                name,
                instantiate_config(
                    display,
                    getattr(self, name, None),
                    kw.pop(name, {})))
        
        for name, value in kw.items():
            setattr(self, name, value)
    
    shaders_path = None
    shaders_parts = ("GEOMETRY", "VERTEX", "FRAGMENT")
    
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
        src = read_file(name)
        includes = re.findall(rb'^#include  *"(.*)"', src, re.MULTILINE)
        for name in includes:
            src = re.sub(rb'#include  *"%s"' % (name,), self._load_shader(name.decode("utf-8")), src, re.MULTILINE)
        return src

    def get_shader(self, SHADER, PART):
        part_name = "shader_%s__%s" % (SHADER, PART)
        shader_name = "shader_%s" % (SHADER,)
        if getattr(self, part_name, None) is not None:
            part = getattr(self, part_name)
        elif getattr(self, shader_name, None) is not None:
            part = "%s/%s" % (getattr(self, shader_name), PART.lower())
        else:
            part = "%s/%s" % (SHADER.lower(), PART.lower())
        if "." not in part.split("/")[-1]:
            part = "%s.glsl" % (part,)
        if "://" not in part and not part.startswith("./") and not part.startswith("../") and not part.startswith("~"):
            part = "%s/%s" % (self.shaders_path, part)
        return part
    
    def activate_shaders(self):
        shaders = set()
        for name in dir(self):
            if name.startswith("shader_"):
                shaders.add(name[len("shader_"):].split("__")[0])
                
        for SHADER in shaders:
            for PART in self.shaders_parts:
                self.display.root["IG_SHADER_%s_%s" % (SHADER, PART)] = self.load_shader(self.get_shader(SHADER, PART))

        if shaders:
            self.display.root["IG_SHADERS"] = ["IG_SHADER_%s" % shader for shader in shaders]

    def activate_properties(self):
        for name in dir(self):
            if name.startswith("root_"):
                value = getattr(self, name)
                if value is not None:
                    self.display.root[name[len("root_"):]] = value

    def activate(self):
        self.activate_shaders()
        self.activate_properties()

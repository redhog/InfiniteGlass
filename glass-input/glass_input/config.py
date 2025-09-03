import InfiniteGlass
import Xlib.X
import Xlib.Xcursorfont
import Xlib.keysymdef.miscellany
import Xlib.ext.xinput
import numpy
import os
import os.path
import datetime
import importlib
import traceback
import operator
import types
import pkg_resources
import yaml
from .mode import Mode

class Config(object):
    def __init__(self, display, config = None):
        self.display = display
        self.config = {}
        self.functions = {}
        self.modes = {}

        if config is None:
            config = os.environ.get("GLASS_INPUT_CONFIG", "~/.config/glass/input.yml")
        if isinstance(config, str):
            with open(os.path.expanduser(config)) as f:
                config = yaml.load(f, Loader=yaml.SafeLoader)

        self.config = config
                
        for module_name in self.config.get("imports", []):
            module = importlib.import_module(module_name)
            module_functions = {
                name: getattr(module, name)
                for name in dir(module)
                if isinstance(getattr(module, name), types.FunctionType)
            }
            module_modes = {
                name: getattr(module, name)
                for name in dir(module)
                if isinstance(getattr(module, name), type) and issubclass(getattr(module, name), Mode)
            }
            self.functions.update(module_functions)
            self.modes.update(module_modes)

        self.push_by_name("base_mode")

    def reload(self):
        self.__init__(self.display)
        
    def push(self, Mode, **kw):
        InfiniteGlass.DEBUG("modes.push", "PUSH %s.%s: %s\n" % (Mode.__module__, Mode.__name__, kw.get("name", kw)))
        if not hasattr(self.display, "input_stack"):
            self.display.input_stack = []
        mode = Mode(config=self, **kw)
        self.display.input_stack.append(mode)
        try:
            mode.enter()
        except:
            self.pop()
            raise

    def push_by_name(self, name, **kw):
        mode = self.config["modes"][name]
        name = next(iter(mode.keys()))
        args = next(iter(mode.values()))
        self.push(self.modes[name], **args, **kw)

    def pop(self):
        res = self.display.input_stack.pop()
        InfiniteGlass.DEBUG("modes.pop", "POP %s\n" % res)
        res.exit()
        return res

    def handle_event(self, event):
        InfiniteGlass.DEBUG("event", "HANDLE %s\n" % event)
        mode = self.display.input_stack[-1]
        if mode.handle(event):
            InfiniteGlass.DEBUG("event", "        BY %s\n" % (mode,))
            return True
        InfiniteGlass.DEBUG("event", "        UNHANDLED\n")
        return False

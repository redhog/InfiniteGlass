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
import importlib.metadata
from .mode import Mode

class Config(object):
    def __init__(self, display, config = None):
        self.display = display
        self.config = {}

        if config is None:
            config = os.environ.get("GLASS_INPUT_CONFIG", "~/.config/glass/input.yml")
        if isinstance(config, str):
            with open(os.path.expanduser(config)) as f:
                config = yaml.load(f, Loader=yaml.SafeLoader)

        self.config = config
        
        self.modes = {entry_point.name: entry_point.load()
                      for entry_point
                      in importlib.metadata.entry_points(group="InfiniteGlass.modes")}

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
            return mode
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
        name = "event.property" if event["PropertyNotify"] else "event"
        InfiniteGlass.DEBUG(name, "HANDLE %s\n" % event)
        mode = self.display.input_stack[-1]
        if mode.handle(event):
            InfiniteGlass.DEBUG(name, "        BY %s\n" % (mode,))
            return True
        InfiniteGlass.DEBUG(name, "        UNHANDLED\n")
        return False

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
from .event_state import EventStatePattern
from .basemode import BaseMode


class Mode(BaseMode):
    def __init__(self, config, **kw):
        self.config = config
        BaseMode.__init__(self, config.display, **kw)
        self.keymap_compiled = self.compile_keymap(self.keymap)

    def compile_action_keymap(self, action):
        if isinstance(action, dict) and "keymap" in action:
            action = dict(action)
            action["keymap"] = self.compile_keymap(action.pop("keymap"))
        return action
            
    def compile_keymap(self, keymap):
        return [(EventStatePattern(eventfilter,
                                   self.display,
                                   self.state,
                                   self.config.config),
                 self.compile_action_keymap(action))
                for eventfilter, action in keymap.items()]
            
    def enter(self):
        self.window = self.get_event_window(self.first_event)
        self.x = 0
        self.y = 0
        if self.window:
            self.orig_window_coords = self.window.get("IG_COORDS", None)
            self.orig_window_size = self.window.get("IG_SIZE", None)
        else:
            self.orig_window_coords = None
            self.orig_window_size = None
        self.orig_view = self.display.root.get("IG_VIEW_DESKTOP_VIEW", None)
        self.orig_size = self.display.root.get("IG_VIEW_DESKTOP_SIZE", None)
        self.state['start'] = datetime.datetime.now()
        if hasattr(self, "load"):
            self.action("load", self.load, None)

    def exit(self):
        pass

    def handle(self, event, keymap=None):
        self.last_event = event

        name = "handle.property" if event["PropertyNotify"] else "handle"
        InfiniteGlass.DEBUG(name, "Handle %s %s\n" % (self, event))
        if keymap is None:
            keymap = self.keymap_compiled
        for eventfilter, action in keymap:
            try:
                if not event[eventfilter]:
                    continue
            except Exception as e:
                InfiniteGlass.ERROR("eventfilter", "%s => %s\n%s\n" % (eventfilter, eventfilter, e))
                raise
            try:
                self.action(eventfilter, action, event)
            except Exception as e:
                InfiniteGlass.ERROR("action", "%s\n%s\n" % (e, traceback.format_exc()))
            return True
        return False

    def action(self, eventfilter, action, event, name=None):
        InfiniteGlass.DEBUG("action", "Action %s.%s [%s]\n" % (self, name if name else action, eventfilter))
        if isinstance(action, (tuple, list)):
            for item in action:
                self.action(eventfilter, item, event)
        else:
            args = {}
            if isinstance(action, dict):
                args = next(iter(action.values()))
                action = next(iter(action.keys()))
                if not isinstance(args, dict):
                    args = {"value": args}
                
            if not isinstance(action, str):
                raise Exception("Unknown action type for %s: %s\n" % (eventfilter, action))

            if action in self.config.config["modes"]:
                self.action(eventfilter, self.config.config["modes"][action], event, name=action)
            elif action in self.config.modes:
                mode = self.config.push(self.config.modes[action], first_event=event, last_event=event, name=name, **args)
                mode.handle(event)
            elif action in self.config.functions:
                InfiniteGlass.DEBUG("final_action", "Function call %s(%s)\n" % (action, args))
                self.config.functions[action](self, event, **args)
            else:
                raise Exception("Unknown action for %s: %s\n" % (eventfilter, action))

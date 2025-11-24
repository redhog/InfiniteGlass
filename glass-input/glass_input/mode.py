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
        self.window = self.get_window(event=self.first_event)
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
        self.run(
            action,
            name="%s [%s]\n" % (name if name else action,
                                eventfilter),
            event=event)
        
    def call_action(self, action, args, event, name=None, **kw):
        if action in self.config.config["modes"]:
            self.run(self.config.config["modes"][action], event=event, name=action, **kw)
        elif action in self.config.modes:
            mode = self.config.push(
                self.config.modes[action],
                first_event=event,
                last_event=event,
                name=name,
                **args)
            mode.handle(event)
        else:
            InfiniteGlass.action.ActionRunner.call_action(self, action, args, event=event, name=name, **kw)

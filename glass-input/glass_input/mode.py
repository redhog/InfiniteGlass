import InfiniteGlass
import Xlib.X
import Xlib.Xcursorfont
import Xlib.keysymdef.miscellany
import Xlib.ext.xinput
import numpy
import os.path
import sys
import pkg_resources
import json
import math
import datetime
import importlib
import traceback

config = {}

def push(display, Mode, **kw):
    InfiniteGlass.DEBUG("modes.push", "PUSH %s\n" % Mode)
    if not hasattr(display, "input_stack"):
        display.input_stack = []
    mode = Mode(display=display, **kw)
    display.input_stack.append(mode)
    if not mode.enter():
        pop(display)
        return False
    return True

def push_config(display, config, **kw):
    cfg = dict(config)
    cfg.update(kw)
    mod, cls = cfg.pop("class").rsplit(".", 1)
    cls = getattr(importlib.import_module(mod), cls)
    return push(display, cls, **cfg)

def push_by_name(display, name, **kw):
    return push_config(display, config[name], **kw)

def pop(display):
    res = display.input_stack.pop()
    InfiniteGlass.DEBUG("modes.pop", "POP %s\n" % res)
    res.exit()
    return res

def handle_event(display, event):
    InfiniteGlass.DEBUG("event", "HANDLE %s\n" % event)
    for i in range(len(display.input_stack)-1, -1, -1):
        mode = display.input_stack[i]
        if mode.handle(event):
            InfiniteGlass.DEBUG("event", "        BY %s %s\n" % (i, mode))
            return True
    InfiniteGlass.DEBUG("event", "        UNHANDLED\n")
    return False

def view_to_space(screen, size, screenx, screeny):
    screeny = screeny - size[1] # FIXME: Merge into matrix...
    screen2space = numpy.array(((screen[2]/size[0],0,0,screen[0]),
                                (0,-screen[3]/size[1],0,screen[1]),
                                (0,0,1,0),
                                (0,0,0,1)))
    space = numpy.array((screenx, screeny, 0., 1.))
    out = screen2space.dot(space)
    return out[:2]

class Mode(object):
    def __init__(self, **kw):
        self.timers = {}
        for key, value in kw.items():
            setattr(self, key, value)

    def enter(self):
        self.timers['start'] = datetime.datetime.now()
        return True
    
    def exit(self):
        pass
    
    def handle(self, event):
        now = datetime.datetime.now()
        for key, value in self.keymap.items():
            timefilters = []
            filters = []
            for item in key.split(","):
                if '>' in item:
                    name, t = item.split('>')
                    timefilters.append((name, False, float(t)))
                elif '<' in item:
                    name, t = item.split('<')
                    timefilters.append((name, True, float(t)))
                else:
                    filters.append(item)
            for name, lt, t in timefilters:
                print("TIMEFILTER",
                      lt != ((now - self.timers[name]).total_seconds() < t),
                      lt, (now - self.timers[name]).total_seconds() < t, (now - self.timers[name]).total_seconds(), t)
                if lt != ((now - self.timers[name]).total_seconds() < t):
                    continue
            if not event[filters]:
                continue
            try:
                self.action(key, value, event)
            except Exception as e:
                print(e)
                traceback.print_exc()
            return True
        return True

    def action(self, key, action, event):
        if isinstance(action, (tuple, list)):
            for item in action:
                self.action(key, item, event)
        elif isinstance(action, dict):
            if "class" in action:
                if push_config(self.display, action, first_event=event, last_event=event):
                    handle_event(self.display, event)
            elif "shell" in action:
                os.system(action["shell"])
            elif "reset" in action:
                self.timers[action['reset']] = datetime.datetime.now()                
            else:
                InfiniteGlass.DEBUG("error", "Unknown action parameters: %s" % (action,))
        elif hasattr(self, action):
            getattr(self, action)(event)
        elif action in config:
            self.action(key, config[action], event)
        else:
            InfiniteGlass.DEBUG("error", "Unknown action for %s: %\n" % (key, action)) 
    
    def pop(self, event):
        pop(self.display)
    
    def get_active_window(self):
        pointer = self.display.root.query_pointer()

        try:
            views = {self.display.root[name + "_LAYER"]: (self.display.root[name + "_VIEW"],
                                                          self.display.root[name + "_SIZE"])
                     for name in self.display.root["IG_VIEWS"]}
        except KeyError:
            return None
        spacecoords = {layer: view_to_space(view, size, pointer.root_x, pointer.root_y)
                       for layer, (view, size) in views.items()}
        for child in self.display.root.query_tree().children:
            coords = child.get("IG_COORDS", None)
            if coords is None: continue
            layer = child.get("IG_LAYER", "IG_LAYER_DESKTOP")
            pointer = spacecoords[layer]
            if (pointer[0] >= coords[0]
                and pointer[0] <= coords[0] + coords[2]
                and pointer[1] <= coords[1]
                and pointer[1] >= coords[1] - coords[3]):
                return child
        return None

    def get_event_window(self, event):
        if event == "ButtonPress":
            return self.get_active_window()
        else:
            focus = self.display.get_input_focus().focus
            if focus == Xlib.X.PointerRoot:
                return None
            return focus

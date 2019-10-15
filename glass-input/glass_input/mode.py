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

def push_by_name(display, name, **kw):
    mod, cls = config[name]["class"].rsplit(".", 1)
    cls = getattr(importlib.import_module(mod), cls)
    return push(display, cls, keymap=config[name]["keymap"], **kw)
    
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
        for key, value in kw.items():
            setattr(self, key, value)

    def enter(self):
        self.start_time = datetime.datetime.now()
        return True
    
    def exit(self):
        pass
    
    def handle(self, event):
        time = datetime.datetime.now() - self.start_time
        for key, value in self.keymap.items():
            key = key.split(",")
            timefilters = [item for item in key if item.startswith("@")]
            key = [item for item in key if not item.startswith("@")]
            if timefilters:
                if time.total_seconds() < float(timefilters[0][1:]):
                    continue
            if not event[key]:
                continue
            try:
                self.action(key, value, event)
            except Exception as e:
                print(e)
                traceback.print_exc()
            return True
        return True

    def action(self, key, name, event):
        if isinstance(name, (tuple, list)):
            for item in name:
                self.action(key, item, event)
        elif hasattr(self, name):
            getattr(self, name)(event)
        elif name in config:
            if "class" in config[name]:
                if push_by_name(self.display, name, first_event=event, last_event=event):
                    handle_event(self.display, event)
            elif "shell" in config[name]:
                os.system(config[name]["shell"])
            else:
                InfiniteGlass.DEBUG("error", "Unknown action parameters for %s" % (name,))
        else:
            InfiniteGlass.DEBUG("error", "Unknown action for %s: %\n" % (key, name)) 
    
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

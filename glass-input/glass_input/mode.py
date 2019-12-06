import InfiniteGlass
import Xlib.X
import Xlib.Xcursorfont
import Xlib.keysymdef.miscellany
import Xlib.ext.xinput
import numpy
import os.path
import datetime
import importlib
import traceback
import operator

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
    for i in range(len(display.input_stack) - 1, -1, -1):
        mode = display.input_stack[i]
        if mode.handle(event):
            InfiniteGlass.DEBUG("event", "        BY %s %s\n" % (i, mode))
            return True
    InfiniteGlass.DEBUG("event", "        UNHANDLED\n")
    return False

def view_to_space(screen, size, screenx, screeny):
    screeny = screeny - size[1] # FIXME: Merge into matrix...
    screen2space = numpy.array(((screen[2] / size[0], 0, 0, screen[0]),
                                (0, -screen[3] / size[1], 0, screen[1]),
                                (0, 0, 1, 0),
                                (0, 0, 0, 1)))
    space = numpy.array((screenx, screeny, 0., 1.))
    out = screen2space.dot(space)
    return out[:2]

def modulo(a, b):
    return a % b == 0

class Mode(object):
    def __init__(self, **kw):
        self.first_event = None
        self.state = {}
        for key, value in kw.items():
            setattr(self, key, value)

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
        return True

    def exit(self):
        pass

    def handle_state_filter(self, eventfilter):
        filters = []
        statefilters = []
        for item in eventfilter:
            if '==' in item:
                name, value = item.split('==')
                statefilters.append((name, operator.eq, float(value)))
            elif '%' in item:
                name, value = item.split('%')
                statefilters.append((name, modulo, int(value)))
            elif '>=' in item:
                name, value = item.split('>=')
                statefilters.append((name, operator.ge, float(value)))
            elif '<=' in item:
                name, value = item.split('<=')
                statefilters.append((name, operator.le, float(value)))
            elif '>' in item:
                name, value = item.split('>')
                statefilters.append((name, operator.gt, float(value)))
            elif '<' in item:
                name, value = item.split('<')
                statefilters.append((name, operator.lt, float(value)))
            else:
                filters.append(item)
        now = datetime.datetime.now()
        for name, op, value in statefilters:
            cvalue = self.state.get(name, 0)
            if isinstance(cvalue, datetime.datetime):
                cvalue = (now - cvalue).total_seconds()
            # print("%s %s %s == %s" % (cvalue, op, value, op(cvalue, value)))
            if not op(cvalue, value):
                return None
        return filters

    def handle(self, event, keymap=None):
        if keymap is None:
            keymap = self.keymap
        for eventfilter, action in keymap.items():
            filters = self.handle_state_filter(eventfilter.split(","))
            if filters is None or filters and not event[filters]:
                continue
            try:
                self.action(eventfilter, action, event)
            except Exception as e:
                print(e)
                traceback.print_exc()
            return True
        return True

    def action(self, eventfilter, action, event):
        if isinstance(action, (tuple, list)):
            for item in action:
                self.action(eventfilter, item, event)
        elif isinstance(action, dict):
            if "class" in action:
                if push_config(self.display, action, first_event=event, last_event=event):
                    handle_event(self.display, event)
            elif "keymap" in action:
                self.handle(event, keymap=action["keymap"])
            elif "shell" in action:
                os.system(action["shell"])
            elif "timer" in action:
                name = action['timer']
                self.state[name] = datetime.datetime.now()
            elif "counter" in action:
                name = action['counter']
                self.state[name] = 0
            elif "inc" in action:
                name = action['inc']
                self.state[name] = self.state.get(name, 0) + 1
            elif len(action.keys()) == 1 and hasattr(self, next(iter(action.keys()))):
                name = next(iter(action.keys()))
                getattr(self, name)(event, **action[name])
            else:
                InfiniteGlass.DEBUG("error", "Unknown action parameters: %s" % (action,))
        elif isinstance(action, str) and hasattr(self, action):
            getattr(self, action)(event)
        elif isinstance(action, str) and action in config:
            self.action(eventfilter, config[action], event)
        else:
            InfiniteGlass.DEBUG("error", "Unknown action for %s: %s\n" % (eventfilter, action))

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
            if child.get_attributes().map_state != Xlib.X.IsViewable: continue
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

    def get_windows(self, view, margin=0.01):
        visible = []
        invisible = []
        for child in self.display.root.query_tree().children:
            if child.get_attributes().map_state != Xlib.X.IsViewable:
                continue

            child = child.find_client_window()
            if not child: continue
            coords = child["IG_COORDS"]

            # Margins to not get stuck due to rounding errors of
            # windows that sit right on the edge...
            marginx = view[2] * margin
            marginy = view[3] * margin
            if (    coords[0] + marginx >= view[0]
                and coords[0] + coords[2] - marginx <= view[0] + view[2]
                and coords[1] - coords[3] + marginy >= view[1]
                and coords[1] - marginy <= view[1] + view[3]):
                visible.append((child, coords))
            else:
                invisible.append((child, coords))
        return visible, invisible

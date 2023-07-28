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

config = {}
functions = {}

def load_config():
    configpath = os.path.expanduser(os.environ.get("GLASS_INPUT_CONFIG", "~/.config/glass/input.yml"))
    with open(configpath) as f:
        set_config(yaml.load(f, Loader=yaml.SafeLoader))

def set_config(cfg):
    global config
    config = cfg    
    for module_name in config.get("imports", []):
        module = importlib.import_module(module_name)
        importlib.reload(module)
        functions.update({
            name: getattr(module, name)
            for name in dir(module)
            if isinstance(getattr(module, name), types.FunctionType)
        })

def push(display, Mode, **kw):
    InfiniteGlass.DEBUG("modes.push", "PUSH %s: %s\n" % (Mode, kw))
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
    return push_config(display, config["modes"][name], **kw)

def pop(display):
    res = display.input_stack.pop()
    InfiniteGlass.DEBUG("modes.pop", "POP %s\n" % res)
    res.exit()
    return res

def handle_event(display, event):
    InfiniteGlass.DEBUG("event", "HANDLE %s\n" % event)
    mode = display.input_stack[-1]
    if mode.handle(event):
        InfiniteGlass.DEBUG("event", "        BY %s\n" % (mode,))
        return True
    InfiniteGlass.DEBUG("event", "        UNHANDLED\n")
    return False

def modulo(a, b):
    return a % b == 0

class Formatter(object):
    def __init__(self, obj):
        self.obj = obj

    def __getitem__(self, name):
        res = self.obj[name]
        if isinstance(res, Xlib.display.drawable.Window):
            return res.__window__()
        return res

class Mode(object):
    def __init__(self, **kw):
        self.first_event = None
        self.last_event = None
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
        if hasattr(self, "load"):
            self.action("load", self.load, None)
        return True

    def get_event_window(self, event=None):
        event = event or self.last_event
        if event == "ClientMessage":
            return event.window
        return InfiniteGlass.windows.get_event_window(self.display, event)
    
    def exit(self):
        pass

    def handle_state_filter(self, eventfilter):
        eventfilter = eventfilter.split(",")
        filters = []
        statefilters = []
        def parse(items, negate=False):
            for item in items:
                if '==' in item:
                    name, value = item.split('==')
                    statefilters.append((name, operator.ne if negate else operator.eq, float(value)))
                elif '%' in item:
                    name, value = item.split('%')
                    statefilters.append((name, modulo, int(value)))
                elif '>=' in item:
                    name, value = item.split('>=')
                    statefilters.append((name, operator.lt if negate else operator.ge, float(value)))
                elif '<=' in item:
                    name, value = item.split('<=')
                    statefilters.append((name, operator.gt if negate else operator.le, float(value)))
                elif '>' in item:
                    name, value = item.split('>')
                    statefilters.append((name, operator.le if negate else operator.gt, float(value)))
                elif '<' in item:
                    name, value = item.split('<')
                    statefilters.append((name, operator.ge if negate else operator.lt, float(value)))
                elif item in config.get("eventfilters", {}):
                    parse(config["eventfilters"][item].split(","), negate)
                elif item.startswith("!") and item[1:] in config.get("eventfilters", {}):
                    parse(config["eventfilters"][item[1:]].split(","), not negate)
                else:
                    if negate:
                        if item.startswith("!"):
                            item = item[1:]
                        else:
                            item = "!" + item
                    filters.append(item)
        parse(eventfilter)
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
        self.last_event = event
        
        if keymap is None:
            keymap = self.keymap
        for eventfilter, action in keymap.items():
            filters = self.handle_state_filter(eventfilter)
            try:
                if filters is None or filters and not event[filters]:
                    continue
            except Exception as e:
                print(eventfilter, "=>", filters)
                print(e)
                raise
            try:
                self.action(eventfilter, action, event)
            except Exception as e:
                print(e)
                traceback.print_exc()
            return True
        return False

    def action(self, eventfilter, action, event, **kw):
        InfiniteGlass.DEBUG("action", "Action %s(%s) [%s]\n" % (action, kw, eventfilter))
        if isinstance(action, (tuple, list)):
            for item in action:
                self.action(eventfilter, item, event)
        elif isinstance(action, dict):
            if "class" in action:
                if push_config(self.display, action, first_event=event, last_event=event, **kw):
                    handle_event(self.display, event)
            elif "keymap" in action:
                self.handle(event, keymap=action["keymap"])
            elif "shell" in action:
                cmd = action["shell"] % Formatter(self)
                InfiniteGlass.DEBUG("final_action", "Shell command %s\n" % (cmd,))
                os.system(cmd)
            elif "timer" in action:
                name = action['timer']
                self.state[name] = datetime.datetime.now()
            elif "counter" in action:
                name = action['counter']
                self.state[name] = 0
            elif "inc" in action:
                name = action['inc']
                self.state[name] = self.state.get(name, 0) + 1
            elif len(action.keys()) == 1:
                name = next(iter(action.keys()))
                self.action(eventfilter, name, event, **action[name])
            else:
                InfiniteGlass.DEBUG("error", "Unknown action parameters: %s\n" % (action,))
        elif isinstance(action, str) and action in config["modes"]:
            self.action(eventfilter, config["modes"][action], event, **kw)
        elif isinstance(action, str) and hasattr(self, action):
            getattr(self, action)(event, **kw)
        elif isinstance(action, str) and action in functions:
            InfiniteGlass.DEBUG("final_action", "Function call %s(%s)\n" % (action, kw))
            functions[action](self, event, **kw)
        else:
            InfiniteGlass.DEBUG("error", "Unknown action for %s: %s\n" % (eventfilter, action))

    def pop(self, event):
        pop(self.display)

    def __getitem__(self, name):
        if name in self.state:
            return self.state[name]
        if hasattr(self, name):
            return getattr(self, name)
        raise KeyError

    @property
    def last_event_window(self):
        return self.get_event_window(self.last_event)

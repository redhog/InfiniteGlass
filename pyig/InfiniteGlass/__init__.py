import Xlib.display
import Xlib.X
import Xlib.xobject.drawable
import Xlib.protocol.event
import sys
import struct
import contextlib

from Xlib.display import Display

orig_display_init = Xlib.display.Display.__init__
def display_init(self, *arg, **kw):
    orig_display_init(self, *arg, **kw)
    self.display.real_display = self
Xlib.display.Display.__init__ = display_init
        
def parse_value(self, value):
    if isinstance(value, (tuple, list)):
        items = value
    elif isinstance(value, str):
        items = value.split(" ")
    else:
        items = [value]
    if isinstance(value, str):
        if value[0] in '0123456789-':
            if "." in value:
                items = [float(item) for item in items]
            else:
                items = [int(item) for item in items]
    if isinstance(items[0], Xlib.xobject.drawable.Window):
        itemtype = self.display.get_atom("WINDOW")
        items = [item.__window__() for item in items]
    elif isinstance(items[0], int):
        itemtype = self.display.get_atom("INTEGER")
    elif isinstance(items[0], float):
        itemtype = self.display.get_atom("FLOAT")
    elif items[0].startswith("@"):
        itemtype = self.display.get_atom("STRING")
        res = []
        for item in items:
            with open(value[1:], "rb") as f:
                res.append(f.read())
        items = res
    else:
        itemtype = self.display.get_atom("ATOM")
        items = [self.display.get_atom(item) for item in items]
    return itemtype, items

def format_value(self, value):
    itemtype, items = parse_value(self, value)

    if isinstance(items[0], int):
        items = struct.pack("<" + "i" * len(items), *items)
    elif isinstance(items[0], float):
        items = struct.pack("<" + "f" * len(items), *items)
    elif isinstance(items[0], bytes):
        items = b''.join(items)
    else:
        items = struct.pack("<" + "l" * len(items), *items)
    return itemtype, items

def window_setitem(self, key, value):
    key = self.display.get_atom(key)
    itemtype, items = format_value(self, value)
    format = 32
    if itemtype == self.display.get_atom("STRING"):
        format = 8
    self.change_property(key, itemtype, format, items)
Xlib.xobject.drawable.Window.__setitem__ = window_setitem

def window_keys(self):
    return [self.display.real_display.get_atom_name(atom) for atom in self.list_properties()]
Xlib.xobject.drawable.Window.keys = window_keys

def window_getitem(self, name):
    res = self.get_property(self.display.get_atom(name), Xlib.X.AnyPropertyType, 0, 100000)
    if res is None:
        raise KeyError("Window %s has no property %s" % (self.__window__(), name))
    property_type = self.display.real_display.get_atom_name(res.property_type)
    res = res.value
    if property_type == "ATOM":
        res = [self.display.real_display.get_atom_name(item) for item in res]
    if property_type == "FLOAT":
        res = struct.unpack("<" + "f" * len(res), res.tobytes())
    if property_type == "WINDOW":
        res = [self.display.real_display.create_resource_object("window", item) for item in res]
    if len(res) == 1:
        res = res[0]
    return res
Xlib.xobject.drawable.Window.__getitem__ = window_getitem

def window_items(self):
    return [(key, self[key]) for key in self.keys()]
Xlib.xobject.drawable.Window.items = window_items

event_mask_map = {
    "MotionNotify": ("ButtonMotionMask","Button1MotionMask","Button2MotionMask","Button3MotionMask","Button4MotionMask","Button5MotionMask"),
    "ButtonPress": "ButtonPressMask",
    "ButtonRelease": "ButtonReleaseMask",
    "ColormapNotify": "ColormapChangeMask",
    "EnterNotify": "EnterWindowMask",
    "LeaveNotify": "LeaveWindowMask",
    "Expose": "ExposureMask",
    "NoExpose": "ExposureMask",
    "FocusIn": "FocusChangeMask",
    "FocusOut": "FocusChangeMask",
    "KeymapNotify": "KeymapStateMask",
    "KeyPress": "KeyPressMask",
    "ReleasePress": "KeyReleaseMask",
    "MotionNotify": "PointerMotionMask",
    "PropertyNotify": "PropertyChangeMask",
    "CirculateNotify": ("StructureNotifyMask", "SubstructureNotifyMask"),
    "ConfigureNotify": ("StructureNotifyMask", "SubstructureNotifyMask"),
    "CreateNotify": "SubstructureNotifyMask",
    "DestroyNotify": ("StructureNotifyMask", "SubstructureNotifyMask"),
    "GravityNotify": ("StructureNotifyMask", "SubstructureNotifyMask"),
    "MapNotify": ("StructureNotifyMask", "SubstructureNotifyMask"),
    "ReparentNotify": ("StructureNotifyMask", "SubstructureNotifyMask"),
    "UnmapNotify": ("StructureNotifyMask", "SubstructureNotifyMask"),
    "ResizeRequest": "ResizeRedirectMask",
    "CirculateRequest": "SubstructureRedirectMask",
    "ConfigureRequest": "SubstructureRedirectMask",
    "MapRequest": "SubstructureRedirectMask",
    "VisibilityNotify": "VisibilityChangeMask"
}
event_mask_map_inv = {}
for key, value in event_mask_map.items():
    if not isinstance(value, tuple): value = (value,)
    for item in value:
        if item in event_mask_map_inv:
            event_mask_map_inv[item] = tuple(set(event_mask_map_inv[item]).union((key,)))
        event_mask_map_inv[item] = (key,)

eventhandlers = []

def on_event(self, any_window=False, event=None, mask=None, **kw):
    def parse(value):
        value = parse_value(self, value)[1]
        if len(value) == 1:
            return value[0]
        return value
    kw = {key: parse(value)
          for key, value in kw.items()}
    def wrapper(fn):
        e = event
        m = mask
        if fn.__name__.endswith("Mask"):
            m = fn.__name__
        else:
            e = fn.__name__
        if m is None:
            m = event_mask_map[e]
            if isinstance(m, tuple): m = m[0]
        if e is None:
            e = event_mask_map_inv[m]
            if isinstance(e, tuple): e = e[0]
        e = getattr(Xlib.X, e)
        self.change_attributes(event_mask = self.get_attributes().your_event_mask | getattr(Xlib.X, m))
        def handler(event):
            if hasattr(event, "window") and event.window.__window__() != self.__window__(): return False
            if event.type != e: return False
            for name, value in kw.items():
                if not hasattr(event, name): return False
                if "mask" in name.lower():
                    if getattr(event, name) & value != value: return False
                else:
                    if getattr(event, name) != value: return False
            fn(self, event)
            return True
        eventhandlers.append(handler)
        return handler
    return wrapper

Xlib.xobject.drawable.Window.on = on_event

def display_enter(self):
    return self
Xlib.display.Display.__enter__ = display_enter
def display_exit(self, exctype, exc, tr):
    if exc is not None:
        raise exc
    self.flush()
    while eventhandlers:
        event = self.next_event()
        for handler in eventhandlers:
            if handler(event):
                break
        self.flush()
Xlib.display.Display.__exit__ = display_exit


def window_require(self, prop):
    def wrapper(fn):    
        @self.on(atom=prop)
        def PropertyNotify(win, event):
            print("NNNNNNNNNNNNNNNNNNNN REQUIRE NOTIFY", event)
            eventhandlers.remove(PropertyNotify)
            fn(self, win[prop])
    return wrapper
Xlib.xobject.drawable.Window.require = window_require


@property
def display_root(self):
    return self.screen(0).root
Xlib.display.Display.root = display_root

_old_create_window = Xlib.xobject.drawable.Window.create_window
def create_window(self, x=0, y=0, width=100, height=100, border_width=0, depth=Xlib.X.CopyFromParent, map=True, *arg, **kw):
    res = _old_create_window(self, x, y, width, height, border_width, depth, *arg, **kw)
    if map:
        res.map()
    return res
    
Xlib.xobject.drawable.Window.create_window = create_window

def window_send(self, window, client_type, *arg, **kw):
    arg = [format_value(self, value) for value in arg]
    format = 32
    if arg and arg[0][0] == self.display.get_atom("STRING"):
        format = 8
    data = b''.join(item[1] for item in arg)
    data = data + b'\0' * (20 - len(data))
    event = Xlib.protocol.event.ClientMessage(
        window = window,
        client_type = self.display.get_atom(client_type),
        data = (format, data))
    self.send_event(event, **kw)
Xlib.xobject.drawable.Window.send = window_send


# with Display() as display:
#     root = display.root

#     root["NANANANA1"] = [1, 2, 3]

#     print(root["NANANANA1"])
    
#     window = root.create_window()

#     window["FOOOOO"] = ["BAR", "HEHE"]
#     window["FIEEEE"] = 34.56

#     @window.on()
#     def ButtonPress(event):
#         print("XXXXXXXXXXXXXXX", event)

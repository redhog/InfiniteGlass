import Xlib.display
import Xlib.X
import Xlib.xobject.drawable
import sys
import struct
import contextlib

from Xlib.display import Display

def window_setitem(self, key, value):
    key = self.display.get_atom(key)
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
    if isinstance(items[0], int):
        itemtype = self.display.get_atom("INTEGER")
        items = struct.pack("<" + "i" * len(items), *items)
    elif isinstance(items[0], float):
        itemtype = self.display.get_atom("FLOAT")
        items = struct.pack("<" + "f" * len(items), *items)
    elif items[0].startswith("@"):
        itemtype = self.display.get_atom("STRING")
        res = []
        for item in items:
            with open(value[1:], "rb") as f:
                res.append(f.read())
        items = ''.join(res)
    else:
        itemtype = self.display.get_atom("ATOM")
        items = [self.display.get_atom(item) for item in items]
        items = struct.pack("<" + "l" * len(items), *items)
    self.change_property(key, itemtype, 32, items)
Xlib.xobject.drawable.Window.__setitem__ = window_setitem

def window_keys(self):
    display = Xlib.display.Display(self.display.get_display_name())
    return [display.get_atom_name(atom) for atom in self.list_properties()]
Xlib.xobject.drawable.Window.keys = window_keys

def window_getitem(self, name):
    display = Xlib.display.Display(self.display.get_display_name())
    res = self.get_property(self.display.get_atom(name), Xlib.X.AnyPropertyType, 0, 100000)
    property_type = display.get_atom_name(res.property_type)
    res = res.value
    if property_type == "ATOM":
        res = [display.get_atom_name(item) for item in res]
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

def on_event(self, event=None, mask=None, **kw):
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
            if event.type != e: return False
            for name, value in kw.items():
                if not hasattr(event, name): return False
                if "mask" in name.lower():
                    if not getattr(event, name) & value != value: return False
                else:
                    if getattr(event, name) != value: return False
            fn(event)
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

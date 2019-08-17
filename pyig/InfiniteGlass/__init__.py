import Xlib.display
import Xlib.X
import Xlib.xobject.drawable
import Xlib.protocol.event
import sys
import struct
import contextlib
import importlib
import array

from Xlib.display import Display

keysymdef = importlib.__import__("Xlib.keysymdef", fromlist="*")

keysyms = {keyname: getattr(dictionary, keyname) 
           for dictionary in [getattr(keysymdef, name) for name in keysymdef.__all__]
           for keyname in dir(dictionary)
           if not keyname.startswith("_")}

orig_display_init = Xlib.display.Display.__init__
def display_init(self, *arg, **kw):
    self.eventhandlers = []
    self.eventhandlerstack = []
    orig_display_init(self, *arg, **kw)
    self.display.real_display = self
Xlib.display.Display.__init__ = display_init
        
def parse_value(self, value):
    if isinstance(value, (tuple, list, array.array)):
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
        itemtype = self.get_atom("WINDOW")
        items = [item.__window__() for item in items]
    elif isinstance(items[0], int):
        itemtype = self.get_atom("INTEGER")
    elif isinstance(items[0], float):
        itemtype = self.get_atom("FLOAT")
    elif items[0].startswith("@"):
        itemtype = self.get_atom("STRING")
        res = []
        for item in items:
            with open(value[1:], "rb") as f:
                res.append(f.read())
        items = res
    elif items[0] in keysyms:
        itemtype = self.get_atom("KEYCODE")
        items = [self.keycode(item) for item in items]
    elif hasattr(Xlib.X, items[0]):
        itemtype = self.get_atom("XCONST")
        items = [getattr(Xlib.X, item) for item in items]
    else:
        itemtype = self.get_atom("ATOM")
        items = [self.get_atom(item) for item in items]
    return itemtype, items

def format_value(self, value):
    itemtype, items = parse_value(self.display, value)

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

def unpack_value(self, value_type, value):
    if value_type == "ATOM":
        value = self.get_atom_name(value)
    if value_type == "FLOAT":
        value = struct.unpack("<f", struct.pack("<i", value))[0]
    if value_type == "WINDOW":
        value = self.create_resource_object("window", value)
    return value

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


def display_on_event(self, event=None, mask=None, **kw):
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
        if m is None and fn.__name__.endswith("Mask"):
            m = fn.__name__
        elif e is None:
            e = fn.__name__
        if m is None:
            m = event_mask_map[e]
            if isinstance(m, tuple): m = m[0]
        if e is None:
            e = event_mask_map_inv[m]
            if isinstance(e, tuple): e = e[0]
        e = getattr(Xlib.X, e)
        def handler(event):
            if event.type != e: return False
            for name, value in kw.items():
                if not hasattr(event, name): return False
                if "mask" in name.lower():
                    if getattr(event, name) & value != value: return False
                else:
                    if getattr(event, name) != value: return False
            fn(self, event)
            return True
        self.eventhandlers.append(handler)
        return handler
    return wrapper
Xlib.display.Display.on = display_on_event
        
def window_on_event(self, event=None, mask=None, **kw):
    def wrapper(fn):
        e = event
        m = mask
        if fn.__name__.endswith("Mask"):
            m = fn.__name__
        else:
            e = fn.__name__
        if m is None:
            m = event_mask_map[e]
        self.change_attributes(event_mask = self.get_attributes().your_event_mask | getattr(Xlib.X, m))
        @self.display.real_display.on(event=e, mask=m, **kw)
        def handler(display, event):
            return fn(self, event)
        return handler
    return wrapper
Xlib.xobject.drawable.Window.on = window_on_event

def display_enter(self):
    self.eventhandlerstack.append(self.eventhandlers)
    self.eventhandlers = []
    return self
Xlib.display.Display.__enter__ = display_enter
def display_exit(self, exctype, exc, tr):
    if exc is not None:
        raise exc
    self.flush()
    while self.eventhandlers:
        event = self.next_event()
        for handler in self.eventhandlers:
            if handler(event):
                break
        self.flush()
    self.eventhandlers = self.eventhandlerstack.pop()
Xlib.display.Display.__exit__ = display_exit

def pop(self):
    self.eventhandlers = []
Xlib.display.Display.pop = pop

def window_require(self, prop):
    def wrapper(fn):
        @self.on(atom=prop)
        def PropertyNotify(win, event):
            self.display.real_display.eventhandlers.remove(PropertyNotify)
            fn(self, win[prop])
        try:
            value = self[prop]
        except KeyError:
            pass
        else:
            self.display.real_display.eventhandlers.remove(PropertyNotify)
            fn(self, value)
            
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

def display_keycode(self, name):
    return self.keysym_to_keycode(keysyms[name])

Xlib.display.Display.keycode = display_keycode

orig_event_eq = Xlib.protocol.rq.Event.__eq__
def event_eq(self, other):
    if isinstance(other, str):
        return self.type == getattr(Xlib.X, other)
    return orig_event_eq(self, other)
Xlib.protocol.rq.Event.__eq__ = event_eq

def client_message_parse(self, *types):
    display = self.window.display.real_display
    format, data = self.data
    return [unpack_value(display, t, value)
            for t, value in zip(types, data)]    
Xlib.protocol.event.ClientMessage.parse = client_message_parse

def keybutton_getitem(self, item):
    if isinstance(item, str):
        if hasattr(Xlib.X, item):
            return self.state & getattr(Xlib.X, item)
        elif item in keysyms:
            return self.detail == self.window.display.real_display.keycode(item)
    else:
        return self.detail == item
Xlib.protocol.event.KeyButtonPointer.__getitem__ = keybutton_getitem


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

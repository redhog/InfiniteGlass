import Xlib.display
import Xlib.X
import Xlib.xobject.drawable
import Xlib.protocol.event
import struct
from . import eventmask
from . import valueencoding
from . import framing
import sys

def window_str(self):
    res = str(self.__window__())
    try:
        if "WM_NAME" in self:
            name = self["WM_NAME"]
            if isinstance(name, list): name = name[0]
            if isinstance(name, bytes): name = name.decode("UTF-8")
            res += ": " + name
    except Exception as e:
        res += " (%s)" % (e,)
    return res
Xlib.xobject.drawable.Window.__str__ = window_str

def window_repr(self):
    return "<window %s>" % window_str(self)
Xlib.xobject.drawable.Window.__repr__ = window_repr

def window_setitem(self, key, value):
    try:
        keyatom = self.display.get_atom(key)
        itemtype, items, fmt = valueencoding.format_value(self, value)
        remaining = []
        if len(items) > 0xfffc:
            remaining = items[0xfffc:]
            items = items[:0xfffc]
        self.change_property(keyatom, itemtype, fmt, items)
        while remaining:
            self.change_property(keyatom, itemtype, fmt, remaining[:0xfffc], Xlib.X.PropModeAppend)
            remaining = remaining[0xfffc:]
    except Exception as e:
        raise Exception("Unable to set %s.%s = %s: %s" % (self, key, repr(value)[:100], e))
Xlib.xobject.drawable.Window.__setitem__ = window_setitem

def window_delitem(self, key):
    try:
        keyatom = self.display.get_atom(key)
        self.delete_property(keyatom)
    except Exception as e:
        raise Exception("Unable to delete %s.%s: %s" % (self, key, e))
Xlib.xobject.drawable.Window.__delitem__ = window_delitem

def window_keys(self):
    return [self.display.real_display.get_atom_name(atom) for atom in self.list_properties()]
Xlib.xobject.drawable.Window.keys = window_keys

def window_getitem(self, name):
    res = self.get_property(self.display.get_atom(name), Xlib.X.AnyPropertyType, 0, 100000)
    if res is None:
        raise KeyError("Window %s has no property %s" % (self.__window__(), name))
    return valueencoding.unpack_values(
        self.display,
        self.display.real_display.get_atom_name(res.property_type),
        res.value)
Xlib.xobject.drawable.Window.__getitem__ = window_getitem

def window_contains(self, name):
    return self.display.get_atom(name) in self.list_properties()
Xlib.xobject.drawable.Window.__contains__ = window_contains

def window_get(self, name, default=None):
    try:
        return self[name]
    except:
        return default
Xlib.xobject.drawable.Window.get = window_get

def window_items(self):
    return [(key, self[key]) for key in self.keys()]
Xlib.xobject.drawable.Window.items = window_items
def window_on_event(self, event=None, mask=None, **kw):
    def wrapper(fn):
        e = event
        m = mask
        if fn.__name__.endswith("Mask"):
            m = fn.__name__
        else:
            e = fn.__name__
        if m is None:
            m = eventmask.event_mask_map[e]
        self.change_attributes(event_mask=self.get_attributes().your_event_mask | getattr(Xlib.X, m))
        @self.display.real_display.on(event=e, mask=m, **kw)
        def handler(display, event):
            win = event.window
            if hasattr(event, "event"): win = event.event
            if self != win:
                return False
            return fn(self, event)
        return handler
    return wrapper
Xlib.xobject.drawable.Window.on = window_on_event

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

_old_create_window = Xlib.xobject.drawable.Window.create_window
def create_window(self, x=0, y=0, width=100, height=100, border_width=0, depth=Xlib.X.CopyFromParent, map=True, *arg, **kw):
    res = _old_create_window(self, x, y, width, height, border_width, depth, *arg, **kw)
    if map:
        res.map()
    return res
Xlib.xobject.drawable.Window.create_window = create_window

def window_send(self, window, client_type, *arg, **kw):
    arg = [valueencoding.format_value(self, value) for value in arg]
    fmt = 32
    if arg:
        fmt = arg[0][2]
    data = b''.join(item[1] for item in arg)
    data = data + b'\0' * (20 - len(data))
    event = Xlib.protocol.event.ClientMessage(
        window=window,
        client_type=self.display.get_atom(client_type),
        data=(fmt, data))
    self.send_event(event, **kw)
Xlib.xobject.drawable.Window.send = window_send

Xlib.xobject.drawable.Window.find_client_window = framing.find_client_window

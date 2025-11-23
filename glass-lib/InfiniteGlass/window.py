import Xlib.display
import Xlib.X
import Xlib.xobject.drawable
import Xlib.protocol.event
import struct
from . import eventmask
from . import valueencoding
from . import framing
from . import utils
import re
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
        raise Exception("Unable to delete %s.%s: %s" % (self, key, e)) from e
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
            if m is None:
                m = fn.__name__
        else:
            if e is None:
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

def window_require_wrapped(self, fn, prop, *props, **kw):    
    @self.on(atom=prop)
    def PropertyNotify(win, event):
        self.display.real_display.off(PropertyNotify)
        value = self[prop]
        if props:
            window_require_wrapped(self, lambda self, *values: fn(self, value, *values), *props, **kw)
        else:
            fn(self, value)
    try:
        value = self[prop]
    except KeyError:
        pass
    else:
        self.display.real_display.off(PropertyNotify)
        if props:
            window_require_wrapped(self, lambda self, *values: fn(self, value, *values), *props, **kw)
        else:
            fn(self, value)

def window_require(self, prop, *props, **kw):
    def wrapper(fn):
        window_require_wrapped(self, fn, prop, *props, **kw)
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

def window_generate_key(self, use):
    return utils.generate_key(self, use)
Xlib.xobject.drawable.Window.generate_key = window_generate_key

orig_window_eq = Xlib.xobject.drawable.Window.__eq__
def window_eq(self, other):
    if isinstance(other, int):
        return self.id == other
    if isinstance(other, str):
        try:
            other = WindowPattern(other, self.display.real_display)
        except ValueError:
            pass
    if isinstance(other, WindowPattern):
        return other.equal(self)
    return orig_window_eq(self, other)
Xlib.xobject.drawable.Window.__eq__ = window_eq

window_attribute_names = [f.name for f in Xlib.protocol.request.GetWindowAttributes._reply.static_fields]

class WindowPatternAST(object):
    def __init__(self, pattern):
        if isinstance(pattern, str):
            pattern = pattern.split(",")
        if not isinstance(pattern, (list, tuple)):
            raise ValueError(type(pattern))
        self.pattern = pattern
        self.keys = []
        self.properties = []
        self.attributes = []
        for item in pattern:
            if "=" not in item:
                include = True
                if item.startswith("!"):
                    item = item[1:]
                    include = False
                self.keys.append((include, item))
            else:
                name, value = item.split("=")
                include = True
                op = "eq"
                if value.startswith("~"):
                    op = "re"
                    value = value[1:]
                if name.endswith("!"):
                    include = False
                    name = name[:-1]
                if name in window_attribute_names:
                    self.attributes.append((include, name, value))
                else:
                    self.properties.append((include, op, name, value))
            
    def __str__(self):
        return ",".join(self.pattern)

GenericEvent = 35

def mostly_equal(a, b):
    if isinstance(a, tuple):
        a = a[1]
    if isinstance(b, tuple):
        b = b[1]
    if isinstance(a, bytes) and isinstance(b, str):
        b = b.encode("UTF-8")
    if isinstance(a, str) and isinstance(b, bytes):
        a = a.encode("UTF-8")
    if not isinstance(a, str) and isinstance(b, str):
        try:
            b = type(a)(b)
        except:
            return False
    if isinstance(a, str) and not isinstance(b, str):
        try:
            a = type(b)(a)
        except:
            return False
    return a == b

def to_str(a):
    if isinstance(a, tuple):
        a = a[1]
    if isinstance(a, bytes):
        return a.decode("UTF-8")
    return str(a)

class WindowPattern(object):
    def __init__(self, pattern, display = None, key_use=[], **context):
        self.display = display
        self.key_use = key_use
        self.context = context
        self.parsed = WindowPatternAST(pattern)
        self.pattern = self.parsed.pattern
        self.keys = [(include, lambda key: not not re.compile(pattern).search(key)) for include, pattern in self.parsed.keys]
        def compile_property(name, op, value):
            if op == "re":
                return lambda win: not not re.compile(value).match(to_str(win[name]))
            elif op == "eq":
                items = [res for item in value.split(",") for res in valueencoding.parse_string_value(self.display, item, **self.context)]
                return lambda win: mostly_equal(win[name], value)
            assert False, "Invalid op: " + op
        self.properties = [(include, compile_property(name, op, value)) for include, op, name, value in self.parsed.properties]

        xinput_opcode = display.query_extension('XInputExtension').major_opcode

        def compile_attribute_value(value):
            if hasattr(Xlib.X, value):
                return getattr(Xlib.X, value)
            elif hasattr(Xlib.ext.ge, value):
                return getattr(Xlib.ext.ge, value)
            elif hasattr(Xlib.ext.xinput, value):
                return (GenericEvent, xinput_opcode, getattr(Xlib.ext.xinput, value))
            try:
                if value[0] in '0123456789-':
                    if "." in value:
                        return (float(value),)
                    else:
                        return (int(value),)
            except:
                return value

        def compile_attribute(name, value):
            value = [compile_attribute_value(item) for item in value.split(",")]
            if "mask" in name:
                value = functools.reduce(lambda x, y: x | y, value, 0)
                return lambda attrs: getattr(attrs, name) & value == value
            else:
                value = value[0]
                return lambda attrs: getattr(attrs, name) == value
            
        self.attributes = [(include, compile_attribute(name, value)) for include, name, value in self.parsed.attributes]

    def keys_eq(self, key): 
        for i, k in self.keys:
            if i != k(key):
                return False
        return True
    
    def properties_eq(self, win): 
        for i, p in self.properties:
            if i != p(win):
                return False
        return True

    def attributes_eq(self, atts): 
        for i, a in self.attributes:
            if i != a(attrs):
                return False
        return True

    def equal(self, window):
        if isinstance(window, WindowPattern):
            pass
        else:
            key = window.generate_key(self.key_use) if self.keys else None
            attrs = window.get_attributes() if self.attributes else None
            print("XXXXXXXXXXXX", key, self.pattern)
            return (self.keys_eq(key)
                    and self.properties_eq(window)
                    and self.attributes_eq(attrs))

    def __eq__(self, other):
        if isinstance(other, str):
            other = WindowPattern(other, self.display)
        return self.equal(other)
    
    def __str__(self):
        return ",".join(self.pattern)

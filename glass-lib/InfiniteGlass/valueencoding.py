import Xlib.xobject.drawable
import Xlib.X
import struct
import array
import pkg_resources
from . import keymap

def parse_value(display, value):
    fmt = 32
    if isinstance(value, tuple):
        itemtype, items, fmt = parse_value(display, value[1])
        return display.get_atom(value[0]), items, fmt

    if isinstance(value, (list, array.array)):
        items = value
    elif isinstance(value, str) and "://" not in value:
        items = value.split(" ")
    else:
        items = [value]
    if isinstance(value, str):
        if value[0] in '0123456789-':
            if "." in value:
                items = [float(item) for item in items]
            else:
                items = [int(item) for item in items]
    if isinstance(value, array.array):
        if value.typecode == 'b':
            itemtype = display.get_atom("STRING")
            fmt = 8
        elif value.typecode == 'B':
            itemtype = display.get_atom("STRING")
            fmt = 8
        elif value.typecode == 'h':
            itemtype = display.get_atom("INTEGER")
        elif value.typecode == 'H':
            itemtype = display.get_atom("CARDINAL")
        elif value.typecode == 'i':
            itemtype = display.get_atom("INTEGER")
        elif value.typecode == 'I':
            itemtype = display.get_atom("CARDINAL")
        elif value.typecode == 'l':
            itemtype = display.get_atom("INTEGER")
        elif value.typecode == 'L':
            itemtype = display.get_atom("CARDINAL")
        elif value.typecode == 'f':
            itemtype = display.get_atom("FLOAT")
        else:
            raise ValueError("Unsupported array type")
    elif isinstance(items[0], Xlib.xobject.drawable.Window):
        itemtype = display.get_atom("WINDOW")
        items = [item.__window__() for item in items]
    elif isinstance(items[0], int):
        itemtype = display.get_atom("INTEGER")
    elif isinstance(items[0], float):
        itemtype = display.get_atom("FLOAT")
    elif isinstance(items[0], bytes):
        itemtype = display.get_atom("STRING")
        fmt = 8
    elif items[0].startswith("@") or "://" in items[0]:
        itemtype = display.get_atom("STRING")
        fmt = 8
        res = []
        for item in items:
            if item[0] == "@": item = item[1:]
            urltype = "path"
            if "://" in item:
                urltype, item = item.split("://")

            if urltype == "resource":
                pkg, item = item.split("/", 1)
                with pkg_resources.resource_stream(pkg, item) as f:
                    res.append(f.read())
            elif urltype == "data":
                res.append(item.encode("utf-8"))
            #elif urltype == "path":
            else:
                with open(item, "rb") as f:
                    res.append(f.read())                
        items = res
    elif items[0] in keymap.keysyms:
        itemtype = display.get_atom("KEYCODE")
        items = [display.keycode(item) for item in items]
    elif hasattr(Xlib.X, items[0]):
        itemtype = display.get_atom("XCONST")
        items = [getattr(Xlib.X, item) for item in items]
    else:
        itemtype = display.get_atom("ATOM")
        items = [display.get_atom(item) for item in items]
    return itemtype, items, fmt

def format_value(window, value):
    itemtype, items, fmt = parse_value(window.display, value)

    if itemtype == window.display.get_atom("CARDINAL"):
        items = struct.pack("<" + "I" * len(items), *items)
    elif isinstance(items[0], int):
        items = struct.pack("<" + "i" * len(items), *items)
    elif isinstance(items[0], float):
        items = struct.pack("<" + "f" * len(items), *items)
    elif isinstance(items[0], bytes):
        items = b'\0'.join(items)
    else:
        items = struct.pack("<" + "l" * len(items), *items)
    return itemtype, items, fmt


def unpack_value(display, value_type, value):
    if value_type == "ATOM":
        value = display.get_atom_name(value)
    if value_type == "FLOAT":
        value = struct.unpack("<f", struct.pack("<i", value))[0]
    if value_type == "WINDOW":
        value = display.create_resource_object("window", value)
    return value

def tojson(display):
    def tojson(obj):
        if isinstance(obj, array.array):
            return {"__jsonclass__": ["array", obj.typecode, list(obj)]}
        elif isinstance(obj, bytes):
            try:
                return {"__jsonclass__": ["string", obj.decode("utf-8")]}
            except:
                return {"__jsonclass__": ["base64", base64.b64encode(obj).decode("ascii")]}
        elif type(obj).__name__ == "Window":
            return {"__jsonclass__": ["Window", obj.__window__()]}
        elif isinstance(obj, tuple):
            return {"__jsonclass__": ["tuple", ] + list(obj)}
        return obj
    return tojson
    
def fromjson(display):
    def fromjson(obj):
        if "__jsonclass__" in obj:
            cls = obj.pop("__jsonclass__")
            if cls[0] == "array":
                return array.array(cls[1], cls[2])
            elif cls[0] == "string":
                return cls[1].encode("utf-8")
            elif cls[0] == "base64":
                return base64.b64decode(cls[1])
            elif cls[0] == "Window":
                return display.create_resource_object("window", cls[1])
            elif cls[0] == "tuple":
                return tuple(cls[1:])
        return obj
    return fromjson

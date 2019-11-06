import Xlib.xobject.drawable
import Xlib.X
import struct
import array
from . import keymap 

def parse_value(display, value):
    if isinstance(value, tuple):
        itemtype, items = parse_value(display, value[1])
        return display.get_atom(value[0]), items
    
    if isinstance(value, (list, array.array)):
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
        itemtype = display.get_atom("WINDOW")
        items = [item.__window__() for item in items]
    elif isinstance(items[0], int):
        itemtype = display.get_atom("INTEGER")
    elif isinstance(items[0], float):
        itemtype = display.get_atom("FLOAT")
    elif isinstance(items[0], bytes):
        itemtype = display.get_atom("STRING")
    elif items[0].startswith("@"):
        itemtype = display.get_atom("STRING")
        res = []
        for item in items:
            with open(value[1:], "rb") as f:
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
    return itemtype, items

def format_value(window, value):
    itemtype, items = parse_value(window.display, value)

    if isinstance(items[0], int):
        items = struct.pack("<" + "i" * len(items), *items)
    elif isinstance(items[0], float):
        items = struct.pack("<" + "f" * len(items), *items)
    elif isinstance(items[0], bytes):
        items = b'\0'.join(items)
    else:
        items = struct.pack("<" + "l" * len(items), *items)
    return itemtype, items


def unpack_value(display, value_type, value):
    if value_type == "ATOM":
        value = display.get_atom_name(value)
    if value_type == "FLOAT":
        value = struct.unpack("<f", struct.pack("<i", value))[0]
    if value_type == "WINDOW":
        value = display.create_resource_object("window", value)
    return value

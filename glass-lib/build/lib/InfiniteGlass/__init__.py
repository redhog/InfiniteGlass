import Xlib.display
import Xlib.X
import Xlib.ext.ge
import Xlib.xobject.drawable
import Xlib.protocol.event
import sys
import struct
import contextlib
import importlib
import array
import traceback
import select
import time
import math
from . import display
from . import window
from . import eventmask
from .keymap import *
from .valueencoding import *
from .debug import *

from Xlib.display import Display

def parse_event_pattern(pattern):
    if isinstance(pattern, str):
        pattern = (pattern,)
    if not isinstance(pattern, (list, tuple)):
        return None
    res = {"buttons": [],
           "masks": [],
           "keys": [],
           "types": []}
    for item in pattern:
        include = True
        if item.startswith("!"):
            include = False
            item = item[1:]
        try:
            item = int(item)
        except:
            pass
        if isinstance(item, int):
            res["buttons"].append((include, item))
        elif item.endswith("Mask"):
            res["masks"].append((include, item))
        elif item.startswith("XK_"):
            res["keys"].append((include, item))
        else:
            res["types"].append((include, item))
    return res

orig_event_eq = Xlib.protocol.rq.Event.__eq__
def event_eq(self, other):
    pattern = parse_event_pattern(other)
    if pattern is None:
        return orig_event_eq(self, other)
    for i, t in pattern["types"]:
        if hasattr(Xlib.X, t):
            t = getattr(Xlib.X, t)
        elif hasattr(Xlib.ext.ge, t):
            t = getattr(Xlib.ext.ge, t)
        else:
            raise Exception("Unknown event type specified in on(): %s" % e)
        if i != (self.type == t):
            return False
    if pattern["masks"] and self.state != sum((getattr(Xlib.X, item) for i, item in pattern["masks"] if i), 0):
        return False
    for i, k in pattern["keys"]:
        if i != (self.detail == self.window.display.real_display.keycode(k)):
            return False
    for i, b in pattern["buttons"]:
        if i != (self.detail == b):
            return False
    return True
Xlib.protocol.rq.Event.__eq__ = event_eq

def event_getitem(self, item):
    pattern = parse_event_pattern(item)
    for i, t in pattern["types"]:
        if hasattr(Xlib.X, t):
            t = getattr(Xlib.X, t)
        elif hasattr(Xlib.ext.ge, t):
            t = getattr(Xlib.ext.ge, t)
        else:
            raise Exception("Unknown event type specified in on(): %s" % e)
        if i != (self.type == t):
            return False
    for i, s in pattern["masks"]:
        if i != (not not self.state & getattr(Xlib.X, s)):
            return False
    for i, k in pattern["keys"]:
        if i != (self.detail == self.window.display.real_display.keycode(k)):
            return False
    for i, b in pattern["buttons"]:
        if i != (self.detail == b):
            return False
    return True
Xlib.protocol.rq.Event.__getitem__ = event_getitem

def client_message_parse(self, *types):
    display = self.window.display.real_display
    format, data = self.data
    return [unpack_value(display, t, value)
            for t, value in zip(types, data)]    
Xlib.protocol.event.ClientMessage.parse = client_message_parse

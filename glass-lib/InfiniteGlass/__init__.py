import Xlib.display
import Xlib.X
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
        try:
            item = int(item)
        except:
            pass
        if isinstance(item, int):
            res["buttons"].append(item)
        elif item.endswith("Mask"):
            res["masks"].append(item)
        elif item.startswith("XK_"):
            res["keys"].append(item)
        else:
            res["types"].append(item)
    return res

orig_event_eq = Xlib.protocol.rq.Event.__eq__
def event_eq(self, other):
    pattern = parse_event_pattern(other)
    if pattern is None:
        return orig_event_eq(self, other)
    for t in pattern["types"]:
        if self.type != getattr(Xlib.X, t):
            return False
    if pattern["masks"] and self.state != sum((getattr(Xlib.X, item) for item in pattern["masks"]), 0):
        return False
    for k in pattern["keys"]:
        if self.detail != self.window.display.real_display.keycode(k):
            return False
    for b in pattern["buttons"]:
        if self.detail != b:
            return False
    return True
Xlib.protocol.rq.Event.__eq__ = event_eq

def event_getitem(self, item):
    pattern = parse_event_pattern(item)
    for t in pattern["types"]:
        if self.type != getattr(Xlib.X, t):
            return False
    for s in pattern["masks"]:
        if not self.state & getattr(Xlib.X, s):
            return False
    for k in pattern["keys"]:
        if self.detail != self.window.display.real_display.keycode(k):
            return False
    for b in pattern["buttons"]:
        if self.detail != b:
            return False
    return True
Xlib.protocol.rq.Event.__getitem__ = event_getitem

def client_message_parse(self, *types):
    display = self.window.display.real_display
    format, data = self.data
    return [unpack_value(display, t, value)
            for t, value in zip(types, data)]    
Xlib.protocol.event.ClientMessage.parse = client_message_parse

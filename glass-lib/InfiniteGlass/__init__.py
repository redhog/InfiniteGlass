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

from Xlib.display import Display

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

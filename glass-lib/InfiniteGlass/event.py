import Xlib.ext.ge
import Xlib.xobject.drawable
import Xlib.protocol.event
import Xlib.protocol.rq

class EventPattern(object):
    def __init__(self, pattern, display):
        if isinstance(pattern, str):
            pattern = pattern.split(",")
        if not isinstance(pattern, (list, tuple)):
            raise ValueError(type(pattern))
        self.display = display
        self.pattern = pattern
        self.buttons = []
        self.masks = []
        self.keys = []
        self.types = []
        self.flags = []
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
                self.buttons.append((include, item))
            elif item.endswith("Mask"):
                self.masks.append((include, getattr(Xlib.X, item)))
            elif item.startswith("XK_"):
                self.keys.append((include, self.display.keycode(item)))
            elif item == "AutoRepeat":
                self.flags.append((include, item))
            else:
                if hasattr(Xlib.X, item):
                    item = getattr(Xlib.X, item)
                elif hasattr(Xlib.ext.ge, item):
                    item = getattr(Xlib.ext.ge, item)
                else:
                    raise Exception("Unknown event type specified in on(): %s" % t)
                self.types.append((include, item))
        self.mask_sum = sum((item
                             for i, item in self.masks
                             if i),
                            0)
    def type_eq(self, event):
        for i, t in self.types:
            if i != (event.type == t):
                return False
        return True

    def mask_eq(self, event):
        return not self.masks or event.state == self.mask_sum

    def mask_contains(self, event):
        for i, s in self.masks:
            if i != (not not event.state & s):
                return False
        return True
    
    def keys_eq(self, event): 
        for i, k in self.keys:
            if i != (event.detail == k):
                return False
        return True

    def buttons_eq(self, event):
        for i, b in self.buttons:
            if i != (event.detail == b):
                return False
        return True
    
    def flags_eq(self, event):
        for i, f in self.flags:
            if i != (hasattr(event, f)):
                return False
        return True
    
    def equal(self, event):
        return (self.type_eq(event)
                and self.mask_eq(event)
                and self.keys_eq(event)
                and self.buttons_eq(event)
                and self.flags_eq(event))
        
    def contained_by(self, event):
        return (self.type_eq(event)
                and self.mask_contains(event)
                and self.keys_eq(event)
                and self.buttons_eq(event)
                and self.flags_eq(event))

    def __str__(self):
        return ",".join(self.pattern)
        
orig_event_eq = Xlib.protocol.rq.Event.__eq__
def event_eq(self, other):
    if not isinstance(other, EventPattern):
        try:
            other = EventPattern(other, self.window.display.real_display)
        except ValueError:
            return orig_event_eq(self, other)
    return other.equal(self)
Xlib.protocol.rq.Event.__eq__ = event_eq

def event_getitem(self, item):
    if not isinstance(item, EventPattern):
        item = EventPattern(item, self.window.display.real_display)
    return item.contained_by(self)
Xlib.protocol.rq.Event.__getitem__ = event_getitem

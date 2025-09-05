import Xlib.ext.ge
import Xlib.xobject.drawable
import Xlib.protocol.event
import Xlib.protocol.rq

class EventPatternAST(object):
    def __init__(self, pattern):
        if isinstance(pattern, str):
            pattern = pattern.split(",")
        if not isinstance(pattern, (list, tuple)):
            raise ValueError(type(pattern))
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
                self.masks.append((include, item))
            elif item.startswith("XK_"):
                self.keys.append((include, item))
            elif item == "AutoRepeat":
                self.flags.append((include, item))
            else:
                self.types.append((include, item))
    def __str__(self):
        return ",".join(self.pattern)


class EventPattern(object):
    def __init__(self, pattern, display = None):
        self.display = display
        self.parsed = EventPatternAST(pattern)
        self.pattern = self.parsed.pattern
        self.buttons = self.parsed.buttons
        self.masks = [(include, getattr(Xlib.X, item)) for (include, item) in self.parsed.masks]
        self.keys = [(include, self.display.keycode(item)) for (include, item) in self.parsed.keys]
        def compile_type(item):
            if hasattr(Xlib.X, item):
                return getattr(Xlib.X, item)
            elif hasattr(Xlib.ext.ge, item):
                return getattr(Xlib.ext.ge, item)
        self.types = [(include, compile_type(item)) for (include, item) in self.parsed.types]
        self.flags = self.parsed.flags
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
        if isinstance(event, EventPattern):
            return (not (set(event.types) - set(self.types))
                    and (set(event.masks) == set(self.masks))
                    and not (set(event.keys) - set(self.keys))
                    and not (set(event.buttons) - set(self.buttons))
                    and not (set(event.flags) - set(self.flags)))
        else:
            return (self.type_eq(event)
                    and self.mask_eq(event)
                    and self.keys_eq(event)
                    and self.buttons_eq(event)
                    and self.flags_eq(event))
        
    def contained_by(self, event):
        if isinstance(event, EventPattern):
            return (not (set(event.types) - set(self.types))
                    and not (set(event.masks) - set(self.masks))
                    and not (set(event.keys) - set(self.keys))
                    and not (set(event.buttons) - set(self.buttons))
                    and not (set(event.flags) - set(self.flags)))
        else:
            return (self.type_eq(event)
                    and self.mask_contains(event)
                    and self.keys_eq(event)
                    and self.buttons_eq(event)
                    and self.flags_eq(event))

    def __getitem__(self, other):
        if isinstance(other, str):
            other = EventPattern(other, self.display)
        return self.contained_by(other)
        
    def __contains__(self, other):
        if isinstance(other, str):
            other = EventPattern(other, self.display)
        # a in b means all flags set by a are also set by b
        return self.contained_by(other)
            
    def __equal__(self, other):
        if isinstance(other, str):
            other = EventPattern(other, self.display)
        return self.equal(other)
    
    def __str__(self):
        return ",".join(self.pattern)
        
orig_event_eq = Xlib.protocol.rq.Event.__eq__
def event_eq(self, other):
    if not isinstance(other, EventPattern):
        try:
            other = EventPattern(other, self.window.display.real_display if hasattr(self, "window") else None)
        except ValueError:
            return orig_event_eq(self, other)
    return other.equal(self)
Xlib.protocol.rq.Event.__eq__ = event_eq

def event_getitem(self, item):
    if not isinstance(item, EventPattern):
        item = EventPattern(item, self.window.display.real_display if hasattr(self, "window") else None)
    return item.contained_by(self)
Xlib.protocol.rq.Event.__getitem__ = event_getitem

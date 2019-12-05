import Xlib.display
import Xlib.X
import Xlib.ext.ge
import Xlib.xobject.drawable
import Xlib.protocol.event
import sys
import traceback
import select
from . import mainloop
from . import eventmask
from . import valueencoding
from . import keymap

orig_display_init = Xlib.display.Display.__init__
def display_init(self, *arg, **kw):
    self.eventhandlers = []
    self.eventhandlerstack = []
    orig_display_init(self, *arg, **kw)
    self.display.real_display = self
    self.mainloop = mainloop.MainLoop()
    @self.mainloop.add(self.fileno())
    def handle_x_event(fd):
        pass
    @self.mainloop.add_hf()
    def handle_x_event():
        for idx in range(self.pending_events()):
            event = self.next_event()
            for handler in self.eventhandlers:
                try:
                    if handler(event):
                        break
                except Exception as e:
                    sys.stderr.write("%s\n" % e)
                    traceback.print_exc(file=sys.stderr)
                    sys.stderr.flush()
            self.flush()
Xlib.display.Display.__init__ = display_init

def fetch_all_pending_events(self):
    while True:
        r, w, e = select.select([self.display.socket], [], [], 0)
        if not r: return
        self.pending_events()
Xlib.display.Display.fetch_all_pending_events = fetch_all_pending_events

orig_next_event = Xlib.display.Display.next_event
def next_event(self):
    event = orig_next_event(self)
    self.fetch_all_pending_events()
    if event.type == Xlib.X.KeyRelease and self.display.event_queue:
        for next_event in self.display.event_queue:
            if next_event.type == Xlib.X.KeyPress and next_event.detail == event.detail:
                event.AutoRepeat = True
                next_event.AutoRepeat = True
                break
    return event
Xlib.display.Display.next_event = next_event

def display_peek_event(self):
    # Fetch any pending events from the server
    if not self.pending_events():
        raise IndexError("No pending events")
    return self.display.event_queue[0]
Xlib.display.Display.peek_event = display_peek_event

def display_on_event(self, event=None, mask=None, **kw):
    def parse(value):
        value = valueencoding.parse_value(self, value)[1]
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
            m = eventmask.event_mask_map[e]
            if isinstance(m, tuple): m = m[0]
        if e is None:
            e = eventmask.event_mask_map_inv[m]
            if isinstance(e, tuple): e = e[0]
        if hasattr(Xlib.X, e):
            e = getattr(Xlib.X, e)
        elif hasattr(Xlib.ext.ge, e):
            e = getattr(Xlib.ext.ge, e)
        else:
            raise Exception("Unknown event type specified in on(): %s" % e)
        def handler(event):
            if event.type != e: return False
            for name, value in kw.items():
                if not hasattr(event, name): return False
                if "mask" in name.lower():
                    if getattr(event, name) & value != value: return False
                else:
                    if getattr(event, name) != value: return False
            return fn(self, event)
        self.eventhandlers.append(handler)
        return handler
    return wrapper
Xlib.display.Display.on = display_on_event

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
        self.mainloop.do()
    self.eventhandlers = self.eventhandlerstack.pop()
Xlib.display.Display.__exit__ = display_exit

def pop(self):
    self.eventhandlers = []
Xlib.display.Display.pop = pop

@property
def display_root(self):
    return self.screen(0).root
Xlib.display.Display.root = display_root

def display_keycode(self, name):
    return self.keysym_to_keycode(keymap.keysyms[name])
Xlib.display.Display.keycode = display_keycode

@property
def display_mask_to_keysym(self):
    return {name: [keymap.symkeys[self.keycode_to_keysym(keycode, 0)]
                   for keycode in mapping
                   if keycode != 0 and self.keycode_to_keysym(keycode, 0) != 0]
            for name, mapping in zip(("ShiftMask", "LockMask", "ControlMask", "Mod1Mask", "Mod2Mask", "Mod3Mask", "Mod4Mask", "Mod5Mask"),
                                    self.get_modifier_mapping())}
Xlib.display.Display.mask_to_keysym = display_mask_to_keysym

import Xlib.X
import InfiniteGlass
from . import pan
import numpy

def set_focus(self, win):
    # FIXME: Don't use CurrentTime as that's not ICCCM compliant...
    if "WM_TAKE_FOCUS" in win.get("WM_PROTOCOLS", []):
        win.send(win, "WM_PROTOCOLS", "WM_TAKE_FOCUS", Xlib.X.CurrentTime)
    else:
        win.set_input_focus(Xlib.X.RevertToNone, Xlib.X.CurrentTime)
    self.display.root["_NET_ACTIVE_WINDOW"] = win
    self.display.flush()
    # Xlib.X.NONE
    self.focus = win
    

def focus_follows_mouse(self, event):
    "Bind this to GenericEventCode to make your keyboard focus follow the mouse"
    pointer = self.display.root.query_pointer()
    win = pointer.child
    if win == getattr(self, "focus", None): return
    if not win: return
    if not InfiniteGlass.windows.is_inside_window(self.display, win): return
    redir = getattr(win, "is_override_redirect", None)
    if redir is None:
        redir = win.get_attributes().override_redirect
        win.is_override_redirect = redir
    if redir: return
    set_focus(self, win)
    

def angular_diff(a, b):
    diff = (a - b) % (2 * numpy.pi)
    if diff <= numpy.pi:
        return diff
    return 2 * numpy.pi - diff

def focus_to_window_to_the_right(self, event):
    "Focus the closest window to the right of the currently focused one"
    focus_to_window_to_the(self, event, 0)
def focus_to_window_above(self, event):
    "Focus the closest window above the currently focused one"
    focus_to_window_to_the(self, event, numpy.pi * 0.5)
def focus_to_window_to_the_left(self, event):
    "Focus the closest window to the left of the currently focused one"
    focus_to_window_to_the(self, event, numpy.pi * 1.)
def focus_to_window_below(self, event):
    "Focus the closest window below the currently focused one"
    focus_to_window_to_the(self, event, numpy.pi * 1.5)

def focus_to_window_to_the(self, event, direction):
    view = list(self.display.root["IG_VIEW_DESKTOP_VIEW"])
    vx = view[0] + view[2] / 2.
    vy = view[1] + view[3] / 2.

    current = self.display.root.get("_NET_ACTIVE_WINDOW", None)
    visible, overlap, invisible = InfiniteGlass.windows.get_windows(self.display, view)
    if current is None:
        if visible: current = visible[0][0]

    if current:
        coords = current["IG_COORDS"]
        cx = coords[0] + coords[2] / 2.
        cy = coords[1] - coords[3] / 2.
    else:
        cx = view[0] + view[2] / 2.
        cy = view[1] + view[3] / 2.
        
    windows = []
    for child, coords in visible + invisible + overlap:
        if child.get("IG_LAYER", "IG_LAYER_DESKTOP") != "IG_LAYER_DESKTOP":
            continue
        if current and child.__window__() == current.__window__(): continue

        x = coords[0] + coords[2] / 2.
        y = coords[1] - coords[3] / 2.

        wdist = numpy.sqrt((x - cx)**2 + (y - cy)**2)
        wdir = angular_diff(numpy.arctan2(y - cy, x - cx), direction) / (numpy.pi / 4)
        if wdir < 1.0:
            windows.append((wdist * (1 + wdir), (x, y), coords, child))
            
    if not windows:
        return

    windows.sort(key=lambda a: a[0])
    dist, center, coords, next_window = windows[0]

    next_is_visible = next_window.__window__() in [w.__window__() for w, c in visible]
    if not next_is_visible:
        view = pan.zoom_to_window_calc(self, view, next_window, view_center=(vx, vy), coords=coords, center=center)
        
        self.display.root["IG_VIEW_DESKTOP_VIEW_ANIMATE"] = view
        self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", self.display.root, "IG_VIEW_DESKTOP_VIEW", .5)

    set_focus(self, next_window)
    InfiniteGlass.DEBUG("window", "Window %s\n" % (next_window,))


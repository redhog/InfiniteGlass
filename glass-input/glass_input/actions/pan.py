import InfiniteGlass
import numpy
from .. import mode
from .. import utils
from . import item_zoom_to
import sys

def pan(self, event, x=None, y=None):
    "Pan the screen a fixed amount in x and/or y"
    if x is None and y is None:
        self.x += event["XK_Left"] - event["XK_Right"]
        self.y += event["XK_Up"] - event["XK_Down"]
    else:
        self.x += x
        self.y += y

    space_orig = InfiniteGlass.coords.view_to_space(self.orig_view, self.orig_size, 0, 0)
    space = InfiniteGlass.coords.view_to_space(self.orig_view, self.orig_size, self.x, self.y)

    view = list(self.orig_view)
    view[0] = self.orig_view[0] - (space[0] - space_orig[0])
    view[1] = self.orig_view[1] - (space[1] - space_orig[1])
    self.display.root["IG_VIEW_DESKTOP_VIEW"] = view

def pan_mouse(self, event):
    "Pan the screen as the mouse moves"
    space_orig = InfiniteGlass.coords.view_to_space(self.orig_view, self.orig_size, self.first_event.root_x, self.first_event.root_y)
    space = InfiniteGlass.coords.view_to_space(self.orig_view, self.orig_size, event.root_x, event.root_y)

    view = list(self.orig_view)
    view[0] = self.orig_view[0] - (space[0] - space_orig[0])
    view[1] = self.orig_view[1] - (space[1] - space_orig[1])

    self.display.root["IG_VIEW_DESKTOP_VIEW"] = view

def zoom_to_window_to_the_right(self, event):
    "Pan/zoom so that one more window is visible to the right"
    zoom_to_window_to_the(self, event, 0)
def zoom_to_window_above(self, event):
    "Pan/zoom so that one more window is visible above"
    zoom_to_window_to_the(self, event, numpy.pi * 0.5)
def zoom_to_window_to_the_left(self, event):
    "Pan/zoom so that one more window is visible to the left"
    zoom_to_window_to_the(self, event, numpy.pi * 1.)
def zoom_to_window_below(self, event):
    "Pan/zoom so that one more window is visible below"
    zoom_to_window_to_the(self, event, numpy.pi * 1.5)

def zoom_to_window_calc(self, view, window, view_center=None, coords=None, center=None):
    view = list(view)
    
    if coords is None:
         coords = window["IG_COORDS"]
    if view_center is None:
        vx = view[0] + view[2] / 2.
        vy = view[1] + view[3] / 2.
    else:
        vx, vy = view_center
    if center is None:
        wx = coords[0] + coords[2] / 2.
        wy = coords[1] - coords[3] / 2.
    else:
        wx, wy = center

    if wx > vx:
        newx = wx + (coords[2] - view[2]) / 2
    else:
        newx = wx - (coords[2] - view[2]) / 2
    if wy > vy:
        newy = wy + (coords[3] - view[3]) / 2
    else:
        newy = wy - (coords[3] - view[3]) / 2

    view[0] = newx - view[2] / 2.
    view[1] = newy - view[3] / 2.            

    return item_zoom_to.adjust_view(self, view, win=window)
    
def zoom_to_window_to_the(self, event, direction):
    print("ZOOM OUT TO WINDOW TO %s" % (direction))
    view = list(self.display.root["IG_VIEW_DESKTOP_VIEW"])
    vx = view[0] + view[2] / 2.
    vy = view[1] + view[3] / 2.

    def angular_diff(a, b):
        diff = (a - b) % (2 * numpy.pi)
        if diff <= numpy.pi:
            return diff
        return 2 * numpy.pi - diff

    windows = []
    visible, overlap, invisible = InfiniteGlass.windows.get_windows(self.display, view)
    for child, coords in invisible + overlap:
        if child.get("IG_LAYER", "IG_LAYER_DESKTOP") != "IG_LAYER_DESKTOP":
            continue

        x = coords[0] + coords[2] / 2.
        y = coords[1] - coords[3] / 2.

        wdist = numpy.sqrt((x - vx)**2 + (y - vy)**2)
        wdir = angular_diff(numpy.arctan2(y - vy, x - vx), direction) / (numpy.pi / 4)
        if wdir < 1.0:
            windows.append((wdist * (1 + wdir), (x, y), coords, child))

    if not windows:
        screeny = numpy.sin(direction)
        screenx = numpy.cos(direction)
        
        view = [view[0] + screenx * view[2], view[1] + screeny * view[3]] + view[2:]

    else:
        windows.sort(key=lambda a: a[0])
        dist, center, coords, next_window = windows[0]
        InfiniteGlass.DEBUG(
            "visible",
            "Visible: %s\n" % (
                ",".join(
                    "%s/%s[%s] @ %s" % (v.get("WM_NAME", None),
                                        v.get("WM_CLASS", None),
                                        v.__window__(),
                                        c)
                    for v, c in visible),))
        InfiniteGlass.DEBUG("next", "Next window %s/%s[%s] @ %s\n" % (
            next_window.get("WM_NAME", None), next_window.get("WM_CLASS", None), next_window.__window__(), coords))
        
        view = zoom_to_window_calc(self, view, next_window, view_center=(vx, vy), coords=coords, center=center)
        
    InfiniteGlass.DEBUG("view", "View %s\n" % (view,))
    self.display.root["IG_VIEW_DESKTOP_VIEW_ANIMATE"] = view
    self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", self.display.root, "IG_VIEW_DESKTOP_VIEW", .5)

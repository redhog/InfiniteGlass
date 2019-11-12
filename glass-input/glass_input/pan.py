import InfiniteGlass
import Xlib.X
import Xlib.Xcursorfont
import Xlib.keysymdef.miscellany
import Xlib.ext.xinput
import numpy
import os.path
import sys
import pkg_resources
import json
import math
import datetime
from . import mode

class PanMode(mode.Mode):
    def __init__(self, **kw):
        mode.Mode.__init__(self, **kw)
        self.x = 0
        self.y = 0
        self.orig_view = self.display.root["IG_VIEW_DESKTOP_VIEW"]
        self.size = self.display.root["IG_VIEW_DESKTOP_SIZE"]
        
    def pan(self, event, x=None, y=None):
        if x is None and y is None:
            self.x += event["XK_Left"] - event["XK_Right"];
            self.y += event["XK_Up"] - event["XK_Down"];
        else:
            self.x += x
            self.y += y
            
        space_orig = mode.view_to_space(self.orig_view, self.size, 0, 0)
        space = mode.view_to_space(self.orig_view, self.size, self.x, self.y)

        view = list(self.orig_view)
        view[0] = self.orig_view[0] - (space[0] - space_orig[0])
        view[1] = self.orig_view[1] - (space[1] - space_orig[1])
        self.display.root["IG_VIEW_DESKTOP_VIEW"] = view
            
    def pan_mouse(self, event):
        space_orig = mode.view_to_space(self.orig_view, self.size, self.first_event.root_x, self.first_event.root_y)
        space = mode.view_to_space(self.orig_view, self.size, event.root_x, event.root_y)

        view = list(self.orig_view)
        view[0] = self.orig_view[0] - (space[0] - space_orig[0])
        view[1] = self.orig_view[1] - (space[1] - space_orig[1])

        self.display.root["IG_VIEW_DESKTOP_VIEW"] = view

    def zoom_to_window_to_the_right(self, event):
        self.zoom_to_window_to_the(event, 0)
    def zoom_to_window_above(self, event):
        self.zoom_to_window_to_the(event, numpy.pi * 0.5)
    def zoom_to_window_to_the_left(self, event):
        self.zoom_to_window_to_the(event, numpy.pi * 1.)
    def zoom_to_window_below(self, event):
        self.zoom_to_window_to_the(event, numpy.pi * 1.5)
        
    def zoom_to_window_to_the(self, event, direction):
        print("ZOOM OUT TO WINDOW TO %s" % (direction))
        view = list(self.display.root["IG_VIEW_DESKTOP_VIEW"])
        vx = view[0] + view[2]/2.
        vy = view[1] + view[3]/2.

        def angular_diff(a, b):
            diff = (a - b) % (2 * numpy.pi) 
            if diff <= numpy.pi:
                return diff
            return 2 * numpy.pi - diff
            
        windows = []
        visible, invisible = self.get_windows(view)
        for child, coords in invisible:
            if child.get("IG_LAYER", "IG_LAYER_DESKTOP") != "IG_LAYER_DESKTOP":
                continue
            
            x = coords[0] + coords[2]/2.
            y = coords[1] - coords[3]/2.

            wdist = numpy.sqrt((x-vx)**2+(y-vy)**2)
            wdir = angular_diff(numpy.arctan2(y-vy, x-vx), direction) / (numpy.pi / 4)
            if wdir < 1.0:
                windows.append((wdist * (1 + wdir), (x, y), coords, child))
                
        if not windows:
            return

        windows.sort(key=lambda a: a[0])
        dist, center, coords, w = windows[0]
        InfiniteGlass.DEBUG(
            "visible",
            "Visible: %s\n" % (
                ",".join(
                    "%s/%s[%s] @ %s" % (v.get("WM_NAME", None),
                                        v.get("WM_CLASS", None),
                                        v.__window__(),
                                        c)
                    for v, c in visible),))
        InfiniteGlass.DEBUG("next", "Next window %s/%s[%s] @ %s\n" % (w.get("WM_NAME", None), w.get("WM_CLASS", None), w.__window__(), coords))
        wx, wy = center
        
        if wx > vx:
            newx = wx + (coords[2] - view[2]) / 2            
        else:
            newx = wx - (coords[2] - view[2]) / 2            
        if wy > vy:
            newy = wy + (coords[3] - view[3]) / 2            
        else:
            newy = wy - (coords[3] - view[3]) / 2
        
        view[0] = newx - view[2]/2.
        view[1] = newy - view[3]/2.

        InfiniteGlass.DEBUG("view", "View %s\n" % (view,))
        self.display.root["IG_VIEW_DESKTOP_VIEW_ANIMATE"] = view
        self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", self.display.root, "IG_VIEW_DESKTOP_VIEW", .5)
        
    

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

class ItemPanMode(mode.Mode):
    def enter(self):
        mode.Mode.enter(self)
        self.window = self.get_event_window(self.first_event)
        if not self.window or self.window == self.display.root:
            mode.pop(self.display)
            mode.push_by_name(self.display, "pan", first_event=self.first_event, last_event=self.last_event)
            return True
        self.x = 0
        self.y = 0
        self.orig_coords = self.window["IG_COORDS"]
        # FIXME: Get the right view...
        self.orig_view = self.display.root["IG_VIEW_DESKTOP_VIEW"]
        self.size = self.display.root["IG_VIEW_DESKTOP_SIZE"]
        return True
    
    def pan(self, event, x=None, y=None):
        if x is None and y is None:
            self.x += event["XK_Right"] - event["XK_Left"]
            self.y += event["XK_Down"] - event["XK_Up"]
        else:
            self.x += x
            self.y += y

        space_orig = mode.view_to_space(self.orig_view, self.size, 0, 0)
        space = mode.view_to_space(self.orig_view, self.size, self.x, self.y)

        coords = list(self.orig_coords)
        coords[0] =  self.orig_coords[0] + (space[0] - space_orig[0])
        coords[1] =  self.orig_coords[1] + (space[1] - space_orig[1])

        self.window["IG_COORDS"] = coords

    def pan_mouse(self, event):
        space_orig = mode.view_to_space(self.orig_view, self.size, self.first_event.root_x, self.first_event.root_y)
        space = mode.view_to_space(self.orig_view, self.size, event.root_x, event.root_y)

        coords = list(self.orig_coords)
        coords[0] =  self.orig_coords[0] + (space[0] - space_orig[0])
        coords[1] =  self.orig_coords[1] + (space[1] - space_orig[1])

        self.window["IG_COORDS"] = coords

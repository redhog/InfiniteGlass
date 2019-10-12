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
        
    def pan(self, event):
        self.x += event["XK_Left"] - event["XK_Right"];
        self.y += event["XK_Up"] - event["XK_Down"];

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

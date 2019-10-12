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

class ItemZoomMode(mode.Mode):
    def enter(self):
        mode.Mode.enter(self)
        self.window = self.get_event_window(self.first_event)
        if not self.window or self.window == self.display.root:
            return False
        return True
    
    def zoom_1_1_to_sreen(self, event):
        size = self.display.root["IG_VIEW_DESKTOP_SIZE"]
        coords = self.window["IG_COORDS"]
        screen = self.display.root["IG_VIEW_DESKTOP_VIEW"]

        geom = [int(size[0] * coords[2]/screen[2]),
                int(size[1] * coords[3]/screen[3])]

        self.window["IG_SIZE_ANIMATE"] = geom
        self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", self.window, "IG_SIZE", .5)

    def zoom_1_1_to_window(self, event):
        winsize = self.window["IG_SIZE"]
        size = self.display.root["IG_VIEW_DESKTOP_SIZE"]
        coords = self.window["IG_COORDS"]
        screen = list(self.display.root["IG_VIEW_DESKTOP_VIEW"])

        screen[2] = size[0] * coords[2]/winsize[0]
        screen[3] = size[1] * coords[3]/winsize[1]
        screen[0] = coords[0] - (screen[2] - coords[2]) / 2.
        screen[1] = coords[1] - (screen[3] + coords[3]) / 2.

        self.display.root["IG_VIEW_DESKTOP_VIEW_ANIMATE"] = screen
        self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", self.display.root, "IG_VIEW_DESKTOP_VIEW", .5)

    def zoom_in(self, event):
        self.window["IG_SIZE"] = [int(item * 1/1.1) for item in self.window["IG_SIZE"]]

    def zoom_out(self, event):
        self.window["IG_SIZE"] = [int(item * 1.1) for item in self.window["IG_SIZE"]]

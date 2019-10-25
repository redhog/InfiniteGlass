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

class ZoomMode(mode.Mode):
    def zoom(self, factor, around_aspect = (0.5, 0.5), around_pos = None, view="IG_VIEW_DESKTOP_VIEW"):
        screen = list(self.display.root[view])
        if around_pos is None:
            around_pos = (screen[0] + screen[2] * around_aspect[0],
                          screen[1] + screen[3] * around_aspect[1])
        else:
            around_aspect = ((around_pos[0] - screen[0]) / screen[2],
                             (around_pos[1] - screen[1]) / screen[3])
        screen[2] *= factor
        screen[3] *= factor
        screen[0] = around_pos[0] - screen[2] * around_aspect[0]
        screen[1] = around_pos[1] - screen[3] * around_aspect[1]
        self.display.root[view] = screen
        
    def zoom_to_window(self, event):
        print("ZOOM IN TO WINDOW")
        win = self.get_active_window()
        old_view = self.display.root["IG_VIEW_DESKTOP_VIEW"]
        view = list(win["IG_COORDS"])
        view[3] = view[2] * old_view[3] / old_view[2]
        view[1] -= view[3]
        self.display.root["IG_VIEW_DESKTOP_VIEW_ANIMATE"] = view
        self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", self.display.root, "IG_VIEW_DESKTOP_VIEW", .5)

    def get_windows(self, view, margin=0.01):
        visible = []
        invisible = []
        for child in self.display.root.query_tree().children:
            if child.get_attributes().map_state != Xlib.X.IsViewable:
                continue
            
            child = child.find_client_window()
            if not child: continue
            coords = child["IG_COORDS"]

            # Margins to not get stuck due to rounding errors of
            # windows that sit right on the edge...
            marginx = view[2] * margin
            marginy = view[3] * margin
            if (    coords[0] + marginx >= view[0]
                and coords[0] + coords[2] - marginx <= view[0] + view[2]
                and coords[1] - coords[3] + marginy >= view[1]
                and coords[1] - marginy <= view[1] + view[3]):
                visible.append((child, coords))
            else:
                invisible.append((child, coords))                
        return visible, invisible
        
    def zoom_to_fewer_windows(self, event, margin=0.01):
        print("ZOOM IN TO FEWER WINDOWS")
        view = list(self.display.root["IG_VIEW_DESKTOP_VIEW"])
        vx = view[0] + view[2]/2.
        vy = view[1] + view[3]/2.
        
        windows = []
        visible, invisible = self.get_windows(view)
        for child, coords in visible:
            x = coords[0] + coords[2]/2.
            y = coords[1] - coords[3]/2.

            d = math.sqrt((x-vx)**2+(y-vy)**2)
            windows.append((d, coords, child))

        if len(windows) == 1:
            return
            
        windows.sort(key=lambda a: a[0])

        ratio = view[2] / view[3]
        
        def get_view(removed = 1):
            xs = [x for d, window, w in windows[:-removed] for x in (window[0], window[0]+window[2])]
            ys = [y for d, window, w in windows[:-removed] for y in (window[1], window[1]-window[3])]
            view = [min(xs), min(ys), max(xs) - min(xs), max(ys) - min(ys)]
            if view[2] / ratio > view[3]:
                view[3] = view[2] / ratio
            else:
                view[2]  = ratio * view[3]
            return view

        for i in range(1, len(windows)):
            new_view = get_view(i)
            if (new_view[2] * (1+margin) < view[2]) or (new_view[3] * (1+margin) < view[3]):
                print("Removed %s windows to reduce width by %s and height by %s" % (i, view[2] - new_view[2], view[3] - new_view[3]))
                InfiniteGlass.DEBUG("view", "View %s\n" % (new_view,))
                #self.display.root["IG_VIEW_DESKTOP_VIEW"] = new_view
                self.display.root["IG_VIEW_DESKTOP_VIEW_ANIMATE"] = new_view
                self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", self.display.root, "IG_VIEW_DESKTOP_VIEW", .5)
                return

        InfiniteGlass.DEBUG("view", "Windows are all overlapping... Not sure what to do...\n")            
                
    def zoom_to_more_windows(self, event):
        print("ZOOM OUT TO MORE WINDOWS")
        view = list(self.display.root["IG_VIEW_DESKTOP_VIEW"])
        vx = view[0] + view[2]/2.
        vy = view[1] + view[3]/2.

        windows = []
        visible, invisible = self.get_windows(view)
        for child, coords in invisible:
            x = coords[0] + coords[2]/2.
            y = coords[1] - coords[3]/2.

            d = math.sqrt((x-vx)**2+(y-vy)**2)
            windows.append((d, coords, child))

        if not windows:
            return

        windows.sort(key=lambda a: a[0])
        d, window, w = windows[0]
        InfiniteGlass.DEBUG("window", "Next window %s/%s[%s] @ %s\n" % (w.get("WM_NAME", None), w.get("WM_CLASS", None), w.__window__(), window))
        
        ratio = view[2] / view[3]

        xs = [window[0], window[0]+window[2]] + [x for w, coords in visible for x in (coords[0], coords[0]+coords[2])]
        ys = [window[1], window[1]-window[3]] + [y for w, coords in visible for y in (coords[1], coords[1]-coords[3])]

        view = [min(xs), min(ys), max(xs) - min(xs), max(ys) - min(ys)]

        InfiniteGlass.DEBUG("view", "View before aspect ratio corr %s\n" % (view,))
        if view[2] / ratio > view[3]:
            view[3] = view[2] / ratio
        else:
            view[2]  = ratio * view[3]
        InfiniteGlass.DEBUG("view", "View %s\n" % (view,))
        self.display.root["IG_VIEW_DESKTOP_VIEW_ANIMATE"] = view
        self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", self.display.root, "IG_VIEW_DESKTOP_VIEW", .5)
        
    def zoom_in(self, event):
        print("ZOOM IN")
        self.zoom(1/1.1)

    def zoom_out(self, event):
        print("ZOOM OUT")
        self.zoom(1.1)

import InfiniteGlass
import math
from .. import mode
from . import zoom
from . import item_zoom_to

def zoom_to_window(self, event):
    "Zoom the screen so that the current window is full-screen"
    print("ZOOM IN TO WINDOW")
    win = self.get_event_window(event)
    old_view = self.display.root["IG_VIEW_DESKTOP_VIEW"]
    view = list(win["IG_COORDS"])
    view[3] = view[2] * old_view[3] / old_view[2]
    view[1] -= view[3]
    self.display.root["IG_VIEW_DESKTOP_VIEW_ANIMATE"] = view
    self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", self.display.root, "IG_VIEW_DESKTOP_VIEW", .5)

def zoom_to_fewer_windows(self, event, margin=0.01):
    "Zoom in the screen so that one fewer window is visible"
    print("ZOOM IN TO FEWER WINDOWS")
    view = list(self.display.root["IG_VIEW_DESKTOP_VIEW"])
    vx = view[0] + view[2] / 2.
    vy = view[1] + view[3] / 2.

    windows = []
    visible, overlap, invisible = InfiniteGlass.windows.get_windows(self.display, view)
    for child, coords in visible:
        d = max(
            math.sqrt((coords[0] - vx)**2 + (coords[1] - vy)**2),
            math.sqrt((coords[0] + coords[2] - vx)**2 + (coords[1] - vy)**2),
            math.sqrt((coords[0] - vx)**2 + (coords[1] - coords[3] - vy)**2),
            math.sqrt((coords[0] + coords[2] - vx)**2 + (coords[1] - coords[3] - vy)**2),)
        windows.append((d, coords, child))

    if len(windows) <= 1:
        return zoom.zoom_in(self, event)

    windows.sort(key=lambda a: a[0])

    ratio = view[2] / view[3]

    def get_view(removed=1):
        xs = [x for d, window, w in windows[:-removed] for x in (window[0], window[0] + window[2])]
        ys = [y for d, window, w in windows[:-removed] for y in (window[1], window[1] - window[3])]
        view = [min(xs), min(ys), max(xs) - min(xs), max(ys) - min(ys)]
        if view[2] / ratio > view[3]:
            view[3] = view[2] / ratio
        else:
            view[2] = ratio * view[3]
        return view

    for i in range(1, len(windows)):
        new_view = get_view(i)
        if (new_view[2] * (1 + margin) < view[2]) or (new_view[3] * (1 + margin) < view[3]):
            adjusted_view = item_zoom_to.adjust_view(self, new_view, windows[-i-1][2])
            if adjusted_view[2] < new_view[2] and adjusted_view[3] < new_view[3]:
                new_view = adjusted_view
            print("Removed %s windows to reduce width by %s and height by %s" % (i, view[2] - new_view[2], view[3] - new_view[3]))
            InfiniteGlass.DEBUG("view", "View %s\n" % (new_view,))
            # self.display.root["IG_VIEW_DESKTOP_VIEW"] = new_view
            self.display.root["IG_VIEW_DESKTOP_VIEW_ANIMATE"] = new_view
            self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", self.display.root, "IG_VIEW_DESKTOP_VIEW", .5)
            return

    InfiniteGlass.DEBUG("view", "Windows are all overlapping... Not sure what to do...\n")

def zoom_to_more_windows(self, event):
    "Zoom out the screen so that one more window is visible"
    print("ZOOM OUT TO MORE WINDOWS")
    view = list(self.display.root["IG_VIEW_DESKTOP_VIEW"])
    vx = view[0] + view[2] / 2.
    vy = view[1] + view[3] / 2.

    windows = []
    visible, overlap, invisible = InfiniteGlass.windows.get_windows(self.display, view)
    for child, coords in invisible + overlap:
        x = coords[0] + coords[2] / 2.
        y = coords[1] - coords[3] / 2.

        d = math.sqrt((x - vx)**2 + (y - vy)**2)
        windows.append((d, coords, child))

    if not windows:
        return zoom.zoom_out(self, event)

    windows.sort(key=lambda a: a[0])
    d, window, next_window = windows[0]
    InfiniteGlass.DEBUG("window", "Next window %s @ %s\n" % (next_window, window))

    ratio = view[2] / view[3]

    xs = [window[0], window[0] + window[2]] + [x for w, coords in visible for x in (coords[0], coords[0] + coords[2])]
    ys = [window[1], window[1] - window[3]] + [y for w, coords in visible for y in (coords[1], coords[1] - coords[3])]

    view = [min(xs), min(ys), max(xs) - min(xs), max(ys) - min(ys)]

    InfiniteGlass.DEBUG("view", "View before aspect ratio corr %s\n" % (view,))
    if view[2] / ratio > view[3]:
        view[3] = view[2] / ratio
    else:
        view[2] = ratio * view[3]
    InfiniteGlass.DEBUG("view", "View after aspect ratio corr %s\n" % (view,))

    view = item_zoom_to.adjust_view(self, view, next_window)
    InfiniteGlass.DEBUG("view", "View after adjustment %s\n" % (view,))
    self.display.root["IG_VIEW_DESKTOP_VIEW_ANIMATE"] = view
    self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", self.display.root, "IG_VIEW_DESKTOP_VIEW", .5)

def zoom_home(self, event):
    "Zoom the screen to its initial position"
    InfiniteGlass.DEBUG("zoom", "ZOOM HOME\n")
    old = self.display.root["IG_VIEW_DESKTOP_VIEW"]
    self.display.root["IG_VIEW_DESKTOP_VIEW_ANIMATE"] = [0., 0., 1., old[3] / old[2]]
    self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", self.display.root, "IG_VIEW_DESKTOP_VIEW", .5)

import InfiniteGlass
import math
from . import zoom
from . import item_zoom_to

def zoom_to_window(self, **kw):
    "Zoom the screen so that the current window is full-screen"
    win = self.get_window(**kw)
    old_view = self.display.root["IG_VIEW_DESKTOP_VIEW"]
    view = list(win["IG_COORDS"])
    view[3] = view[2] * old_view[3] / old_view[2]
    view[1] -= view[3]
    self.display.root["IG_VIEW_DESKTOP_VIEW_ANIMATE"] = view
    self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", self.display.root, "IG_VIEW_DESKTOP_VIEW", .5)

def zoom_to_fewer_windows(self, event, margin=0.01, **kw):
    focus_win = self.get_window(event=event, **kw)
    
    "Zoom in the screen so that one fewer window is visible"
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
    if focus_win is not None:
        # Move the focus window first, so we remove it last...
        focus_win_child = [(d, coords, child) for (d, coords, child) in windows if child == focus_win]
        windows = focus_win_child + [(d, coords, child) for (d, coords, child) in windows if child != focus_win]

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
            InfiniteGlass.DEBUG("view", "Removed %s windows to reduce width by %s and height by %s\n" % (i, view[2] - new_view[2], view[3] - new_view[3]))
            InfiniteGlass.DEBUG("view", "View %s\n" % (new_view,))
            # self.display.root["IG_VIEW_DESKTOP_VIEW"] = new_view
            self.display.root["IG_VIEW_DESKTOP_VIEW_ANIMATE"] = new_view
            self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", self.display.root, "IG_VIEW_DESKTOP_VIEW", .5)
            return

    InfiniteGlass.DEBUG("view", "Windows are all overlapping... Not sure what to do...\n")

def zoom_to_more_windows(self, event):
    "Zoom out the screen so that one more window is visible"
    view = list(self.display.root["IG_VIEW_DESKTOP_VIEW"])
    vx = view[0] + view[2] / 2.
    vy = view[1] + view[3] / 2.

    windows = []
    visible, overlap, invisible = InfiniteGlass.windows.get_windows(self.display, view)
    for child, coords in invisible + overlap:
        d = min(
            math.sqrt((coords[0] - vx)**2 + (coords[1] - vy)**2),
            math.sqrt((coords[0] + coords[2] - vx)**2 + (coords[1] - vy)**2),
            math.sqrt((coords[0] - vx)**2 + (coords[1] - coords[3] - vy)**2),
            math.sqrt((coords[0] + coords[2] - vx)**2 + (coords[1] - coords[3] - vy)**2),)
        windows.append((d, coords, child))

    if not windows:
        return zoom.zoom_out(self, event)

    windows.sort(key=lambda a: a[0])
    d, window, next_window = windows[0]
    InfiniteGlass.DEBUG("window", "Next window %s @ %s\n" % (next_window, window))

    ratio = view[2] / view[3]

    wxs = [x for w, coords in visible for x in (coords[0], coords[0] + coords[2])]
    wys = [y for w, coords in visible for y in (coords[1], coords[1] - coords[3])]
    xs = [window[0], window[0] + window[2]] + wxs
    ys = [window[1], window[1] - window[3]] + wys

    view = [min(xs), min(ys), max(xs) - min(xs), max(ys) - min(ys)]

    InfiniteGlass.DEBUG("view", "View before aspect ratio corr %s\n" % (view,))
    if view[2] / ratio > view[3]:
        view[3] = view[2] / ratio
    else:
        view[2] = ratio * view[3]
    InfiniteGlass.DEBUG("view", "View after aspect ratio corr %s\n" % (view,))

    adjusted_view = item_zoom_to.adjust_view(self, view, next_window)    
    if len(visible):
        visible_view = [min(wxs), min(wys), max(wxs) - min(wxs), max(wys) - min(wys)]
        if (   visible_view[0] >= adjusted_view[0]
            and visible_view[1] >= adjusted_view[1]
            and visible_view[0] + visible_view[2] <= adjusted_view[0] + adjusted_view[2]
            and visible_view[1] + visible_view[3] <= adjusted_view[1] + adjusted_view[3]):
            view = adjusted_view
    else:
        view = adjusted_view

    InfiniteGlass.DEBUG("view", "View after adjustment %s\n" % (view,))
    self.display.root["IG_VIEW_DESKTOP_VIEW_ANIMATE"] = view
    self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", self.display.root, "IG_VIEW_DESKTOP_VIEW", .5)

def zoom_home(self, event):
    "Zoom the screen to its initial position"
    InfiniteGlass.DEBUG("zoom", "ZOOM HOME\n")
    old = self.display.root["IG_VIEW_DESKTOP_VIEW"]
    self.display.root["IG_VIEW_DESKTOP_VIEW_ANIMATE"] = [0., 0., 1., old[3] / old[2]]
    self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", self.display.root, "IG_VIEW_DESKTOP_VIEW", .5)

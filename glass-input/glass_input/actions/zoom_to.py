import InfiniteGlass
import math
from .. import mode

def zoom_to_window(self, event):
    print("ZOOM IN TO WINDOW")
    win = self.get_active_window()
    old_view = self.display.root["IG_VIEW_DESKTOP_VIEW"]
    view = list(win["IG_COORDS"])
    view[3] = view[2] * old_view[3] / old_view[2]
    view[1] -= view[3]
    self.display.root["IG_VIEW_DESKTOP_VIEW_ANIMATE"] = view
    self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", self.display.root, "IG_VIEW_DESKTOP_VIEW", .5)

def zoom_to_fewer_windows(self, event, margin=0.01):
    print("ZOOM IN TO FEWER WINDOWS")
    view = list(self.display.root["IG_VIEW_DESKTOP_VIEW"])
    vx = view[0] + view[2] / 2.
    vy = view[1] + view[3] / 2.

    windows = []
    visible, invisible = self.get_windows(view)
    for child, coords in visible:
        x = coords[0] + coords[2] / 2.
        y = coords[1] - coords[3] / 2.

        d = math.sqrt((x - vx)**2 + (y - vy)**2)
        windows.append((d, coords, child))

    if len(windows) == 1:
        return

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
            print("Removed %s windows to reduce width by %s and height by %s" % (i, view[2] - new_view[2], view[3] - new_view[3]))
            InfiniteGlass.DEBUG("view", "View %s\n" % (new_view,))
            # self.display.root["IG_VIEW_DESKTOP_VIEW"] = new_view
            self.display.root["IG_VIEW_DESKTOP_VIEW_ANIMATE"] = new_view
            self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", self.display.root, "IG_VIEW_DESKTOP_VIEW", .5)
            return

    InfiniteGlass.DEBUG("view", "Windows are all overlapping... Not sure what to do...\n")

def zoom_to_more_windows(self, event):
    print("ZOOM OUT TO MORE WINDOWS")
    view = list(self.display.root["IG_VIEW_DESKTOP_VIEW"])
    vx = view[0] + view[2] / 2.
    vy = view[1] + view[3] / 2.

    windows = []
    visible, invisible = self.get_windows(view)
    for child, coords in invisible:
        x = coords[0] + coords[2] / 2.
        y = coords[1] - coords[3] / 2.

        d = math.sqrt((x - vx)**2 + (y - vy)**2)
        windows.append((d, coords, child))

    if not windows:
        return

    windows.sort(key=lambda a: a[0])
    d, window, w = windows[0]
    InfiniteGlass.DEBUG("window", "Next window %s/%s[%s] @ %s\n" % (w.get("WM_NAME", None), w.get("WM_CLASS", None), w.__window__(), window))

    ratio = view[2] / view[3]

    xs = [window[0], window[0] + window[2]] + [x for w, coords in visible for x in (coords[0], coords[0] + coords[2])]
    ys = [window[1], window[1] - window[3]] + [y for w, coords in visible for y in (coords[1], coords[1] - coords[3])]

    view = [min(xs), min(ys), max(xs) - min(xs), max(ys) - min(ys)]

    InfiniteGlass.DEBUG("view", "View before aspect ratio corr %s\n" % (view,))
    if view[2] / ratio > view[3]:
        view[3] = view[2] / ratio
    else:
        view[2] = ratio * view[3]
    InfiniteGlass.DEBUG("view", "View %s\n" % (view,))
    self.display.root["IG_VIEW_DESKTOP_VIEW_ANIMATE"] = view
    self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", self.display.root, "IG_VIEW_DESKTOP_VIEW", .5)

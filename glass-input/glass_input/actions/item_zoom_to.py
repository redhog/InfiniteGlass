import InfiniteGlass
from .. import mode

def item_zoom_1_1_to_sreen(self, event):
    win = InfiniteGlass.windows.get_active_window()
    if not win or win == self.display.root:
        return

    size = self.display.root["IG_VIEW_DESKTOP_SIZE"]
    coords = win["IG_COORDS"]
    screen = self.display.root["IG_VIEW_DESKTOP_VIEW"]

    geom = [int(size[0] * coords[2] / screen[2]),
            int(size[1] * coords[3] / screen[3])]

    win["IG_SIZE_ANIMATE"] = geom
    self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", win, "IG_SIZE", .5)

def item_zoom_1_1_to_window(self, event):
    win = InfiniteGlass.windows.get_active_window()
    if not win or win == self.display.root:
        return

    winsize = win["IG_SIZE"]
    size = self.display.root["IG_VIEW_DESKTOP_SIZE"]
    coords = win["IG_COORDS"]
    screen = list(self.display.root["IG_VIEW_DESKTOP_VIEW"])

    screen[2] = size[0] * coords[2] / winsize[0]
    screen[3] = size[1] * coords[3] / winsize[1]
    screen[0] = coords[0] - (screen[2] - coords[2]) / 2.
    screen[1] = coords[1] - (screen[3] + coords[3]) / 2.

    self.display.root["IG_VIEW_DESKTOP_VIEW_ANIMATE"] = screen
    self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", self.display.root, "IG_VIEW_DESKTOP_VIEW", .5)

def zoom_1_1_1(self, event):
    # zoom_screen_to_window_and_window_to_screen
    win = InfiniteGlass.windows.get_active_window()
    if not win or win == self.display.root:
        return
    InfiniteGlass.DEBUG("zoom", "zoom_screen_to_window_and_window_to_screen\n")
    size = self.display.root["IG_VIEW_DESKTOP_SIZE"]
    coords = list(win["IG_COORDS"])
    screen = list(self.display.root["IG_VIEW_DESKTOP_VIEW"])

    coords[3] = (size[1] * coords[2]) / size[0]

    screen[2] = coords[2]
    screen[3] = coords[3]
    screen[0] = coords[0]
    screen[1] = coords[1] - screen[3]

    InfiniteGlass.DEBUG("zoom", "    screen=%s geom=%s\n" % (screen, size))

    win["IG_COORDS_ANIMATE"] = coords
    win["IG_SIZE_ANIMATE"] = size
    self.display.root["IG_VIEW_DESKTOP_VIEW_ANIMATE"] = screen
    self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", win, "IG_COORDS", .5)
    self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", win, "IG_SIZE", .5)
    self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", self.display.root, "IG_VIEW_DESKTOP_VIEW", .5)
    self.display.flush()
    self.display.sync()

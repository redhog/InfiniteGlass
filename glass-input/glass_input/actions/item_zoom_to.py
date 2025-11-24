import InfiniteGlass
from .. import utils

def item_zoom_1_1_to_sreen_calc(self, win, size = None, coords = None, view = None):
    size = size or self.display.root["IG_VIEW_DESKTOP_SIZE"]
    coords = coords or win["IG_COORDS"]
    view = view or self.display.root["IG_VIEW_DESKTOP_VIEW"]

    return [int(size[0] * coords[2] / view[2]),
            int(size[1] * coords[3] / view[3])]

def item_zoom_1_1_to_sreen(self, **kw):
    "Set the pixel resolution of the current window so that it matches the space it occupies on the screen"
    win = self.get_window(**kw)
    if not win or win == self.display.root:
        return None

    win["IG_SIZE_ANIMATE"] = item_zoom_1_1_to_sreen_calc(self, win)
    self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", win, "IG_SIZE", .5)

def item_zoom_in_or_1_1(self, **kw):
    "Like item_zoom_1_1_to_sreen, but then keeps zooming in if repeated"
    win = self.get_window(**kw)
    if not win or win == self.display.root:
        return None

    geom = item_zoom_1_1_to_sreen_calc(self, win)
    if win["IG_SIZE"][0] > geom[0] or win["IG_SIZE"][1] > geom[1]:
        win["IG_SIZE_ANIMATE"] = geom
        self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", win, "IG_SIZE", .5)
    else:
        win["IG_SIZE"] = [int(item * 1 / 1.1) for item in win["IG_SIZE"]]

def item_zoom_out_or_1_1(self, **kw):
    "Like item_zoom_1_1_to_sreen, but then keeps zooming out if repeated"
    win = self.get_window(**kw)
    if not win or win == self.display.root:
        return None

    geom = item_zoom_1_1_to_sreen_calc(self, win)
    if win["IG_SIZE"][0] < geom[0] or win["IG_SIZE"][1] < geom[1]:
        win["IG_SIZE_ANIMATE"] = geom
        self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", win, "IG_SIZE", .5)
    else:
        win["IG_SIZE"] = [int(item * 1.1) for item in win["IG_SIZE"]]

def item_zoom_1_1_to_window_calc(self, win=None, winsize=None, coords=None, size=None, screen=None, **kw):
    if win is None:
        win = self.get_window(**kw)
        if not win or win == self.display.root:
            return

    if not winsize: winsize = win["IG_SIZE"]
    if not size: size = self.display.root["IG_VIEW_DESKTOP_SIZE"]
    if not coords: coords = win["IG_COORDS"]
    if not screen: screen = self.display.root["IG_VIEW_DESKTOP_VIEW"]

    screen = list(screen)
    screen[2] = size[0] * coords[2] / winsize[0]
    # We don't do screen[3] = size[1] * coords[3] / winsize[1]
    # as that would let bad aspect ratios on windows blead to the view
    screen[3] = (screen[2] * size[1]) / size[0]
    screen[0] = coords[0] - (screen[2] - coords[2]) / 2.
    screen[1] = coords[1] - (screen[3] + coords[3]) / 2.

    return screen
        
def item_zoom_1_1_to_window(self, win=None, **kw):
    "Zooms the screen so that the pixel resolution of the current window matches the space it occupies on the screen"
    self.display.root["IG_VIEW_DESKTOP_VIEW_ANIMATE"] = item_zoom_1_1_to_window_calc(self, win, **kw)
    self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", self.display.root, "IG_VIEW_DESKTOP_VIEW", .5)

def zoom_1_1_1(self, **kw):
    "Zoom to make the current window full-screen, and change its resolution to the same as the screen"
    win = self.get_window(**kw)
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

def adjust_view(self, view, win=None):
    visible, overlap, invisible = InfiniteGlass.windows.get_windows(self.display, view)

    visible_pixelcontent = [(w, c) for w, c in visible if "IG_CONTENT" not in w]
    
    if win is None:
        for win in visible:
            if "IG_CONTENT" not in win:
                break
        else:
            InfiniteGlass.DEBUG("zoom", "No focus window, and no visible windows without IG_CONTENT to focus instead\n")
            return view

    if "IG_CONTENT" in win:
        return view

    zoomed_view = item_zoom_1_1_to_window_calc(self, win=win, screen=view)
    
    if len(visible_pixelcontent) > 1:
        bbox = utils.bbox([c for w, c in visible])
        InfiniteGlass.DEBUG("zoom", "Visible windows bbox: %s\n" % (bbox,))
        if zoomed_view[2] >= bbox[2] and zoomed_view[3] >= bbox[3]:
            InfiniteGlass.DEBUG("zoom", "Zoom 1-to-1 to %s @ %s (View still contains all visible windows)\n" % (win, win.get("IG_COORDS")))
            view = zoomed_view

        view[0] = bbox[0] - ((view[2] - bbox[2]) / 2)
        view[1] = bbox[1]-bbox[3] - ((view[3] - bbox[3]) / 2)
    else:
        InfiniteGlass.DEBUG("zoom", "Zoom 1-to-1 to %s @ %s (Less than 2 visible windows)\n" % (win, win.get("IG_COORDS")))
        return zoomed_view
            
    InfiniteGlass.DEBUG("zoom", "Zoom-adjusted view: %s\n" % (view,))
        
    # Move the view as long as that makes more windows visible without removing any windows
    while True:
        view[0] = bbox[0] - ((view[2] - bbox[2]) / 2)
        view[1] = bbox[1]-bbox[3] - ((view[3] - bbox[3]) / 2)
        
        new_visible, new_overlap, new_invisible = InfiniteGlass.windows.get_windows(self.display, view)
        visible_set = set(w.__window__() for w, c in visible)
        new_visible_set = set(w.__window__() for w, c in new_visible)
        
        if len(visible_set - new_visible_set) != 0 or len(new_visible_set - visible_set) == 0:
            return view

        visible = new_visible
        bbox = utils.bbox([c for w, c in visible])

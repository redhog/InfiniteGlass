from . import islands
import InfiniteGlass

def item_pan_space(self, space_x, space_y):
    coords = list(self.orig_window_coords)
    coords[0] = self.orig_window_coords[0] + space_x
    coords[1] = self.orig_window_coords[1] + space_y

    self.window["IG_COORDS"] = coords

    if self.window.get("IG_WINDOW_TYPE") == "IG_ISLAND":
        if not hasattr(self, "island_windows"):
            self.island_windows = islands.island_windows(self.display, self.window)
        for window, orig_window_coords in self.island_windows:
           coords = list(orig_window_coords)
           coords[0] = orig_window_coords[0] + space_x
           coords[1] = orig_window_coords[1] + space_y
           window["IG_COORDS"] = coords


def item_pan(self, event, x=None, y=None):
    "Move the current window a fixed amount in x and/or y"
    if x is None and y is None:
        self.x += event["XK_Right"] - event["XK_Left"]
        self.y += event["XK_Down"] - event["XK_Up"]
    else:
        self.x += x
        self.y += y

    space_orig = InfiniteGlass.coords.view_to_space(self.orig_view, self.orig_size, 0, 0)
    space = InfiniteGlass.coords.view_to_space(self.orig_view, self.orig_size, self.x, self.y)

    space_x = space[0] - space_orig[0]
    space_y = space[1] - space_orig[1]
    item_pan_space(self, space_x, space_y)
           
def item_pan_mouse(self, event):
    "Move the current window as the mouse moves"    
    space_orig = InfiniteGlass.coords.view_to_space(self.orig_view, self.orig_size, self.first_event.root_x, self.first_event.root_y)
    space = InfiniteGlass.coords.view_to_space(self.orig_view, self.orig_size, event.root_x, event.root_y)

    space_x = space[0] - space_orig[0]
    space_y = space[1] - space_orig[1]
    item_pan_space(self, space_x, space_y)

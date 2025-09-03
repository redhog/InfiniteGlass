import InfiniteGlass

def item_resize(self, event, x=None, y=None):
    "Resize the current window a fixed amount in x and/or y"
    if x is None and y is None:
        self.x += event["XK_Right"] - event["XK_Left"]
        self.y += event["XK_Down"] - event["XK_Up"]
    else:
        self.x += x
        self.y += y

    space_orig = InfiniteGlass.coords.view_to_space(self.orig_view, self.orig_size, 0, 0)
    space = InfiniteGlass.coords.view_to_space(self.orig_view, self.orig_size, self.x, self.y)

    coords = list(self.orig_window_coords)
    coords[2] = self.orig_window_coords[2] + (space[0] - space_orig[0])
    coords[3] = self.orig_window_coords[3] - (space[1] - space_orig[1])

    self.window["IG_COORDS"] = coords
    self.window["IG_SIZE"] = [int((self.orig_window_size[0] / self.orig_window_coords[2]) * coords[2]),
                              int((self.orig_window_size[0] / self.orig_window_coords[2]) * coords[3])]

def item_resize_mouse(self, event):
    "Resize the current window as the mouse moves"    
    space_orig = InfiniteGlass.coords.view_to_space(self.orig_view, self.orig_size, self.first_event.root_x, self.first_event.root_y)
    space = InfiniteGlass.coords.view_to_space(self.orig_view, self.orig_size, event.root_x, event.root_y)

    coords = list(self.orig_window_coords)
    coords[2] = self.orig_window_coords[2] + (space[0] - space_orig[0])
    coords[3] = self.orig_window_coords[3] - (space[1] - space_orig[1])

    self.window["IG_COORDS"] = coords
    self.window["IG_SIZE"] = [int((self.orig_window_size[0] / self.orig_window_coords[2]) * coords[2]),
                              int((self.orig_window_size[0] / self.orig_window_coords[2]) * coords[3])]

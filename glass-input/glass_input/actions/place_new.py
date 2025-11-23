def place_new_center(self, **kw):
    view = self.display.root["IG_VIEW_DESKTOP_VIEW"]
    window = self.get_window(**kw)
    geom = window.get_geometry()

    root_geom = self.display.root.get_geometry()
    x = view[2] * geom.x / float(root_geom.width)
    width = view[2] * geom.width / float(root_geom.width)
    y = view[3] * geom.y / float(root_geom.height)
    height = view[3] * geom.height / float(root_geom.height)

    if x == 0.0:
        x = (view[2] - width) / 2
    if y == 0.0:
        y = (view[3] - height) / 2

    window["IG_COORDS"] = [view[0] + x, view[1] + view[3] - y, width, height]
    self.display.flush()

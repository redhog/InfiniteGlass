from .. import mode

def item_zoom_in(self, event):
    "Decrease the pixel resolution of the current window"
    self.window["IG_SIZE"] = [int(item * 1 / 1.1) for item in self.window["IG_SIZE"]]

def item_zoom_out(self, event):
    "Increase the pixel resolution of the current window"
    self.window["IG_SIZE"] = [int(item * 1.1) for item in self.window["IG_SIZE"]]

from .. import mode

def item_zoom_in(self, event):
    self.window["IG_SIZE"] = [int(item * 1 / 1.1) for item in self.window["IG_SIZE"]]

def item_zoom_out(self, event):
    self.window["IG_SIZE"] = [int(item * 1.1) for item in self.window["IG_SIZE"]]

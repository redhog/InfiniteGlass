import Xlib.X
import InfiniteGlass


def toggle_overlay(self, event):
    old = self.display.root["IG_VIEW_OVERLAY_VIEW"]
    if old[0] == 0.:
        self.display.root["IG_VIEW_OVERLAY_VIEW_ANIMATE"] = [.2, .2, .6, .6 * old[3] / old[2]]
    else:
        self.display.root["IG_VIEW_OVERLAY_VIEW_ANIMATE"] = [0., 0., 1., old[3] / old[2]]
    self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", self.display.root, "IG_VIEW_OVERLAY_VIEW", .5)

def send_debug(self, event):
    InfiniteGlass.DEBUG("debug", "SENDING DEBUG\n")
    self.display.root.send(
        self.display.root, "IG_DEBUG",
        event_mask=Xlib.X.StructureNotifyMask|Xlib.X.SubstructureRedirectMask)
    self.display.flush()

def send_close(self, event):
    win = self.get_active_window()
    if win and win != self.display.root:
        InfiniteGlass.DEBUG("close", "SENDING CLOSE %s\n" % win)
        win.send(win, "IG_CLOSE", event_mask=Xlib.X.StructureNotifyMask)
        self.display.flush()

def send_sleep(self, event):
    win = self.get_active_window()
    if win and win != self.display.root:
        InfiniteGlass.DEBUG("sleep", "SENDING SLEEP %s\n" % win)
        win.send(win, "IG_SLEEP", event_mask=Xlib.X.StructureNotifyMask)
        self.display.flush()

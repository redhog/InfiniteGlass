import InfiniteGlass
import Xlib.X
import Xlib.Xcursorfont
import Xlib.keysymdef.miscellany
import Xlib.ext.xinput
from . import mode

class GrabbedMode(mode.Mode):
    def __init__(self, **kw):
        mode.Mode.__init__(self, **kw)
        self.display.root.grab_pointer(
            False, Xlib.X.ButtonPressMask | Xlib.X.ButtonReleaseMask | Xlib.X.PointerMotionMask,
            Xlib.X.GrabModeAsync, Xlib.X.GrabModeAsync, self.display.root, self.display.input_cursor, Xlib.X.CurrentTime)
        self.display.root.grab_keyboard(False, Xlib.X.GrabModeAsync, Xlib.X.GrabModeAsync, Xlib.X.CurrentTime)

    def exit(self):
        self.display.ungrab_pointer(Xlib.X.CurrentTime)
        self.display.ungrab_keyboard(Xlib.X.CurrentTime)

    def debugpos(self, event):
        pass
        # self.get_active_window()

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

    def zoom_1_1_1(self, event):
        win = self.get_active_window()
        if win and win != self.display.root:
            # zoom_screen_to_window_and_window_to_screen

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

    def zoom_home(self, event):
        InfiniteGlass.DEBUG("zoom", "ZOOM HOME\n")
        old = self.display.root["IG_VIEW_DESKTOP_VIEW"]
        self.display.root["IG_VIEW_DESKTOP_VIEW_ANIMATE"] = [0., 0., 1., old[3] / old[2]]
        self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", self.display.root, "IG_VIEW_DESKTOP_VIEW", .5)

import Xlib.X
import Xlib.Xcursorfont
import Xlib.keysymdef.miscellany
import Xlib.ext.xinput
from . import mode

class BaseMode(mode.Mode):
    def __init__(self, **kw):
        mode.Mode.__init__(self, **kw)
        for mod in range(0, 2**8):
            self.display.root.grab_key(self.display.keycode("XK_Super_L"),
                                       mod, False, Xlib.X.GrabModeAsync, Xlib.X.GrabModeAsync)
            self.display.root.grab_button(
                Xlib.X.AnyButton, Xlib.X.Mod4Mask | mod, False,
                Xlib.X.ButtonPressMask | Xlib.X.ButtonReleaseMask | Xlib.X.PointerMotionMask,
                Xlib.X.GrabModeAsync, Xlib.X.GrabModeAsync, self.display.root, self.display.input_cursor)
        self.display.root["IG_VIEW_OVERLAY_VIEW"] = [.2, .2, .6, 0.0]

    def focus_follows_mouse(self, event):
        win = self.get_active_window()
        if win:
            win.set_input_focus(Xlib.X.RevertToNone, Xlib.X.CurrentTime)

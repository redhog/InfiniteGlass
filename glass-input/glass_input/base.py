import Xlib.X
import Xlib.Xcursorfont
import Xlib.keysymdef.miscellany
import Xlib.ext.xinput
from .actions import actions
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

        @self.display.root.require("IG_VIEW_OVERLAY_SIZE")
        def overlay_size(root, value):
            actions.toggle_overlay(self, None, False)

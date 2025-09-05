import InfiniteGlass
import Xlib.X
import Xlib.Xcursorfont
import Xlib.keysymdef.miscellany
import Xlib.ext.xinput
from .actions import actions
from . import mode

class BaseMode(mode.Mode):
    grab = []
    
    def enter(self):
        mode.Mode.enter(self)

        for mod in range(0, 2**8):
            for keymap, action in self.keymap_compiled:
                if keymap["KeyPress"]:
                    keys = keymap.keys
                    if not keys:
                        keys = [(True, Xlib.X.AnyKey)]
                    for en, keycode in keymap.keys:
                        if not en: continue
                        self.display.root.grab_key(
                            keycode, keymap.mask_sum | mod, False, Xlib.X.GrabModeAsync, Xlib.X.GrabModeAsync)
                
                elif keymap["ButtonPress"] or keymap["ButtonRelease"]: # FIXME: What about PointerMotion?
                    buttons = keymap.buttons
                    if not buttons:
                        buttons = [(True, Xlib.X.AnyButton)]

                    masks = [InfiniteGlass.eventmask.event_mask_map[t] for (include, t) in keymap.parsed.types]
                    masks = [m if isinstance(m, tuple) else (m,) for m in masks]
                    masks = sum([getattr(Xlib.X, m) for ms in masks for m in ms], 0)
                        
                    for en, button in keymap.buttons:
                        if not en: continue
                        self.display.root.grab_button(
                            button, keymap.mask_sum | mod, False, masks,
                            Xlib.X.GrabModeAsync, Xlib.X.GrabModeAsync,
                            self.display.root, self.display.input_cursor)

        @self.display.root.require("IG_VIEW_OVERLAY_SIZE")
        def overlay_size(root, value):
            @self.display.root.require("IG_ANIMATE")
            def animate_window(root, win):
                self.display.animate_window = win
                actions.toggle_overlay(self, None, False)

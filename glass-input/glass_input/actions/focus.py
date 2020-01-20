import Xlib.X
import InfiniteGlass

def focus_follows_mouse(self, event):
    pointer = self.display.root.query_pointer()
    win = pointer.child
    if win == getattr(self, "focus", None): return
    if not win: return
    if not InfiniteGlass.windows.is_inside_window(self.display, win): return
    
    # FIXME: Don't use CurrentTime as that's not ICCCM compliant...
    if "WM_TAKE_FOCUS" in win.get("WM_PROTOCOLS", []):
        win.send(win, "WM_PROTOCOLS", "WM_TAKE_FOCUS", Xlib.X.CurrentTime)
    else:
        win.set_input_focus(Xlib.X.RevertToNone, Xlib.X.CurrentTime)
    self.display.root["_NET_ACTIVE_WINDOW"] = win
    self.display.flush()
    # Xlib.X.NONE
    self.focus = win

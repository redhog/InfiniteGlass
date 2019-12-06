import Xlib.X

def focus_follows_mouse(self, event):
    win = self.get_active_window()
    if win == getattr(self, "focus", None): return
    if not win: return
    win.set_input_focus(Xlib.X.RevertToNone, Xlib.X.CurrentTime)
    self.display.root["_NET_ACTIVE_WINDOW"] = win
    self.display.flush()
    # Xlib.X.NONE
    self.focus = win

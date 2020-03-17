import Xlib.X

def send_msg(display, win, mask, event):
    if isinstance(win, str):
        if win == "root":
            win = display.root
        else:
            win = display.create_resource_object("window", int(win))
    def conv(item):
        try:
            if "." in item:
                return float(item)
            else:
                return int(item)
        except:
            return item
    event = [conv(item) for item in event]
    print("SEND win=%s, mask=%s %s" % (win, mask, ", ".join(repr(item) for item in event)))
    win.send(win, *event, event_mask=getattr(Xlib.X, mask))
    display.flush()


def get_pointer_window(display):
    def get_pointer_window(cb):
        font = display.open_font('cursor')
        input_cursor = font.create_glyph_cursor(
            font, Xlib.Xcursorfont.box_spiral, Xlib.Xcursorfont.box_spiral + 1,
            (65535, 65535, 65535), (0, 0, 0))
        
        display.root.grab_pointer(
            False, Xlib.X.ButtonPressMask | Xlib.X.ButtonReleaseMask,
            Xlib.X.GrabModeAsync, Xlib.X.GrabModeAsync, display.root, input_cursor, Xlib.X.CurrentTime)

        @display.on()
        def ButtonPress(win, event):
            display.ungrab_pointer(Xlib.X.CurrentTime)
            window = event.child
            window = window.find_client_window()
            cb(window)
    return get_pointer_window

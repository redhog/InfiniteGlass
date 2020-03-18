import Xlib.X

def str_to_win(display, window):
    def str_to_win(fn):
        def handle_window(win):
            if win == "root":
                win = display.root
            elif isinstance(win, (str, int)):
                win = display.create_resource_object("window", int(win))
            fn(win)
        if window == "click":
            @get_pointer_window(display)
            def with_win(win):
                handle_window(win)
        else:
            handle_window(window)        
    return str_to_win
            
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

def send_msg(cb):
    def send_msg(display, win, mask, event):
        @str_to_win(display, win)
        def send_msg(win):
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
            cb()
    return send_msg

import InfiniteGlass
import Xlib.Xcursorfont
import Xlib.X
import pkg_resources

current_win_start = None
current_win = None
current_coord = None


with InfiniteGlass.Display() as display:
    font = display.open_font('cursor')
    display.input_cursor = font.create_glyph_cursor(
        font, Xlib.Xcursorfont.diamond_cross, Xlib.Xcursorfont.diamond_cross+1,
        (65535, 65535, 65535), (0, 0, 0))

    display.notify_motion_window = -1
    @display.root.require("IG_NOTIFY_MOTION")
    def notify_motion(root, win):
        display.notify_motion_window = win
        @win.on()
        def PropertyNotify(win, event):
            global current_coord, current_win_start
            name = display.get_atom_name(event.atom)
            if name != "IG_NOTIFY_MOTION": return
            current_coord = win[name]
            if current_win is not None:
                x = min(current_coord[0], current_win_start[0])
                y = max(current_coord[1], current_win_start[1])
                w = abs(current_coord[0] - current_win_start[0])
                h = abs(current_coord[1] - current_win_start[1])
                current_win["IG_COORDS"]=[x, y, w, h]
                InfiniteGlass.DEBUG("resize", "Resize win %s\n" %(current_win["IG_COORDS"],))
                
    display.root.grab_button(
        Xlib.X.AnyButton, Xlib.X.Mod5Mask, False,
        Xlib.X.ButtonPressMask | Xlib.X.ButtonReleaseMask | Xlib.X.PointerMotionMask,
        Xlib.X.GrabModeAsync, Xlib.X.GrabModeAsync, display.root, display.input_cursor)           

    @display.root.on()
    def ButtonPress(win, event):
        global current_win, current_win_start
        
        current_win = display.root.create_window(map=False)
        current_win["IG_LAYER"]="IG_LAYER_DESKTOP"

        current_win_start = list(current_coord[:2])
        current_win["IG_COORDS"]=current_win_start + [0.01, 0.01]
        InfiniteGlass.DEBUG("new", "New win %s\n" % (current_win["IG_COORDS"],))
        
        with pkg_resources.resource_stream("glass_wintest", "wintest.svg") as f:
            current_win["DISPLAYSVG"]=f.read()

        current_win.map()
        
    @display.root.on()
    def ButtonRelease(win, event):
        global current_win
        
        current_win = None
        
    InfiniteGlass.DEBUG("init", "Wintest started\n")

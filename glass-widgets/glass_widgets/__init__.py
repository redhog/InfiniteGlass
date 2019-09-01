import InfiniteGlass, Xlib.X
import struct
import pkg_resources

def main(*arg, **kw):
    with InfiniteGlass.Display() as display:
        display.root["IG_VIEWS"]=["IG_VIEW_DESKTOP", "IG_VIEW_OVERLAY", "IG_VIEW_MENU"]
        display.root["IG_VIEW_MENU_LAYER"]="IG_LAYER_MENU"
        display.root["IG_VIEW_MENU_VIEW"]=[0.0, 0.0, 1.0, 0.0]
        display.root["IG_VIEW_OVERLAY_LAYER"]="IG_LAYER_OVERLAY"
        display.root["IG_VIEW_OVERLAY_VIEW"]=[0.0, 0.0, 1.0, 0.0]
        display.root["IG_VIEW_DESKTOP_LAYER"]="IG_LAYER_DESKTOP"
        display.root["IG_VIEW_DESKTOP_VIEW"]=[0.0, 0.0, 1.0, 0.0]

        for idx, (icon, zoom) in enumerate((("search-plus", 0.9090909090909091),
                                            ("search-minus", 1.1),
                                            ("search-location", -1.0))):
            w = display.root.create_window()
            w.zoom = zoom
            w["IG_LAYER"]="IG_LAYER_OVERLAY"
            w["IG_COORDS"]=[0.01, 0.60+0.05*idx, 0.05, 0.05]
            with pkg_resources.resource_stream("glass_widgets", "fontawesome-free-5.9.0-desktop/svgs/solid/%s.svg" % icon) as f:
                w["DISPLAYSVG"]=f.read()
            @w.on()
            def ButtonPress(win, event):
                screen = list(display.root["IG_VIEW_DESKTOP_VIEW"])
                if win.zoom == -1.:
                    screen[0] = 0.
                    screen[1] = 0.
                    screen[3] = screen[3] / screen[2]
                    screen[2] = 1.
                else:
                    screen[2] *= win.zoom
                    screen[3] *= win.zoom
                display.root["IG_VIEW_DESKTOP_VIEW"] = screen

        w = display.root.create_window()
        w["IG_LAYER"]="IG_LAYER_OVERLAY"
        w["IG_COORDS"]=[0.01,0.55,0.05,0.05]
        with pkg_resources.resource_stream("glass_widgets", "fontawesome-free-5.9.0-desktop/svgs/solid/eject.svg") as f:
            w["DISPLAYSVG"]=f.read()
        @w.on()
        def ButtonPress(win, event):
            display.root.send(display.root,
                              "IG_EXIT",
                              event_mask=Xlib.X.SubstructureNotifyMask|Xlib.X.SubstructureRedirectMask)

        @display.root.on(mask="PointerMotionMask")
        def ClientMessage(win, event):
            coords = [struct.unpack("<" + "f", struct.pack("<L", event.data[1][i]))
                      for i in range(4)]

            print("MOTION", win, coords)

        print("Widgets started")

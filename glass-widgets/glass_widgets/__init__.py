import InfiniteGlass
import Xlib.X
import struct
import pkg_resources

def main(*arg, **kw):
    with InfiniteGlass.Display() as display:
        for idx, (icon, zoom) in enumerate((("search-plus", 0.9090909090909091),
                                            ("search-minus", 1.1),
                                            ("search-location", -1.0))):
            w = display.root.create_window()
            w.zoom = zoom
            w["WM_NAME"] = icon.encode("utf-8")
            w["WM_CLASS"] = b"glass-widget"
            w["IG_LAYER"] = "IG_LAYER_OVERLAY"
            w["IG_COORDS"] = [0.01, 0.60 + 0.05 * idx, 0.05, 0.05]
            w["_NET_WM_WINDOW_TYPE"] = "_NET_WM_WINDOW_TYPE_DESKTOP"
            with pkg_resources.resource_stream("glass_widgets", "fontawesome-free-5.9.0-desktop/svgs/solid/%s.svg" % icon) as f:
                data = f.read()
                w["IG_CONTENT"] = ("IG_SVG", data)
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
        w["WM_NAME"] = b"eject"
        w["WM_CLASS"] = b"glass-widget"
        w["IG_LAYER"] = "IG_LAYER_OVERLAY"
        w["IG_COORDS"] = [0.01, 0.55, 0.05, 0.05]
        w["_NET_WM_WINDOW_TYPE"] = "_NET_WM_WINDOW_TYPE_DESKTOP"
        with pkg_resources.resource_stream("glass_widgets", "fontawesome-free-5.9.0-desktop/svgs/solid/eject.svg") as f:
            data = f.read()
            w["IG_CONTENT"] = ("IG_SVG", data)
        @w.on()
        def ButtonPress(win, event):
            display.root.send(display.root,
                              "IG_EXIT",
                              event_mask=Xlib.X.SubstructureNotifyMask|Xlib.X.SubstructureRedirectMask)

        @display.root.on(mask="PointerMotionMask")
        def ClientMessage(win, event):
            coords = [struct.unpack("<" + "f", struct.pack("<L", event.data[1][i]))
                      for i in range(4)]

            InfiniteGlass.DEBUG("motion", "MOTION %s %s\n" % (win, coords))

        InfiniteGlass.DEBUG("init", "Widgets started\n")

xterm &
#xclock &
{
python3 <<EOF
import InfiniteGlass, Xlib.X
with InfiniteGlass.Display() as display:
    display.root["IG_VIEWS"]=["IG_VIEW_OVERLAY", "IG_VIEW_DESKTOP"]
    display.root["IG_VIEW_OVERLAY_LAYER"]="IG_LAYER_OVERLAY"
    display.root["IG_VIEW_OVERLAY_VIEW"]=[0.0, 0.0, 1.0, 0.75]
    display.root["IG_VIEW_DESKTOP_LAYER"]="IG_LAYER_DESKTOP"
    display.root["IG_VIEW_DESKTOP_VIEW"]=[0.0, 0.0, 1.0, 0.75]

    for idx, (icon, zoom) in enumerate((("search-plus", 0.9090909090909091),
                                        ("search-minus", 1.1),
                                        ("search-location", -1.0))):
        w = display.root.create_window()
        w.zoom = zoom
        w["IG_LAYER"]="IG_LAYER_OVERLAY"
        w["IG_X"]=0.01; w["IG_Y"]=0.60+0.05*idx; w["IG_W"]=0.05; w["IG_H"]=0.05
        w["DISPLAYSVG"]="@fontawesome-free-5.9.0-desktop/svgs/solid/%s.svg" % icon
        @w.on()
        def ButtonPress(win, event):
            display.root.send(display.root,
                              "IG_ZOOM", "IG_LAYER_DESKTOP", win.zoom,
                              event_mask=Xlib.X.SubstructureNotifyMask|Xlib.X.SubstructureRedirectMask)

    w = display.root.create_window()
    w["IG_LAYER"]="IG_LAYER_OVERLAY"
    w["IG_X"]=0.01; w["IG_Y"]=0.55; w["IG_W"]=0.05; w["IG_H"]=0.05
    w["DISPLAYSVG"]="@fontawesome-free-5.9.0-desktop/svgs/solid/eject.svg"
    @w.on()
    def ButtonPress(win, event):
        display.root.send(display.root,
                          "IG_EXIT",
                          event_mask=Xlib.X.SubstructureNotifyMask|Xlib.X.SubstructureRedirectMask)
EOF
} &
gdbserver :2048 ./build/wm &
while sleep 1; do
  :
done

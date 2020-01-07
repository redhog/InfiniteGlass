import InfiniteGlass
import Xlib.X
import struct
import pkg_resources
import os.path
import yaml
import json

def main(*arg, **kw):
    configpath = os.path.expanduser(os.environ.get("GLASS_WIDGET_CONFIG", "~/.config/glass/widgets.json"))
    with open(configpath) as f:
        config = yaml.load(f, Loader=yaml.SafeLoader)
            
    with InfiniteGlass.Display() as display:
        for name, widget in config["widgets"].items():
            w = display.root.create_window()

            properties = {
                "WM_CLASS": "data://glass-widget",
                "IG_LAYER": "IG_LAYER_OVERLAY",
                "_NET_WM_WINDOW_TYPE": "_NET_WM_WINDOW_TYPE_DESKTOP"
            }
            properties.update(widget["properties"])
            properties = json.loads(
                json.dumps(properties),
                object_hook=InfiniteGlass.fromjson(display))
            
            for key, value in properties.items():
                w[key] = value

            w.widget = widget
            
            @w.on()
            def ButtonPress(win, event):
                win["IG_INPUT_ACTION"] = json.dumps(win.widget["action"]).encode("utf-8")
                display.root.send(display.root,
                                  "IG_INPUT_ACTION", win, "IG_INPUT_ACTION",
                                  event_mask=Xlib.X.SubstructureNotifyMask|Xlib.X.SubstructureRedirectMask)
                display.flush()
                
        InfiniteGlass.DEBUG("init", "Widgets started\n")

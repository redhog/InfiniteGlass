import InfiniteGlass
import Xlib.X
import struct
import pkg_resources
import os.path
import yaml
import json

if os.environ.get("GLASS_DEBUGGER", "") == "rpdb":
    import rpdb
    rpdb.handle_trap()
    rpdb.handle_trap()

def main(*arg, **kw):
    configpath = os.path.expanduser(os.environ.get("GLASS_WIDGET_CONFIG", "~/.config/glass/widgets.yml"))
    with open(configpath) as f:
        config = yaml.load(f, Loader=yaml.SafeLoader)
            
    with InfiniteGlass.Display() as display:
        for widget_type, widgets in config.items():
            for name, widget in widgets.items():
                w = display.root.create_window()

                properties = {
                    "WM_CLASS": "data://glass-widget",
                    "WM_NAME": "data://" + name,
                    "_NET_WM_WINDOW_TYPE": "_NET_WM_WINDOW_TYPE_DESKTOP"
                }
                if widget_type == "widgets":
                    properties["IG_LAYER"] = "IG_LAYER_OVERLAY"
                elif widget_type == "window-decorations":
                    properties["IG_LAYER"] = "IG_LAYER_NONE"
                    properties["IG_ITEM_LAYER"] = "IG_LAYER_DESKTOP"
                    properties["IG_SHADER"] = "IG_SHADER_DECORATION"
                    display.root["IG_WINDOW_DECORATION_" + name.upper()] = ("IG_ITEM", w)
                elif widget_type == "island-decorations":
                    properties["IG_LAYER"] = "IG_LAYER_NONE"
                    properties["IG_ITEM_LAYER"] = "IG_LAYER_ISLAND"
                    properties["IG_SHADER"] = "IG_SHADER_DECORATION"
                    display.root["IG_ISLAND_DECORATION_" + name.upper()] = ("IG_ITEM", w)
                properties.update(widget["properties"])
                properties = json.loads(
                    json.dumps(properties),
                    object_hook=InfiniteGlass.fromjson(display))

                for key, value in properties.items():
                    w[key] = value

                w.widget = widget

                @w.on()
                def ButtonPress(win, event):
                    parent = win.get("IG_PARENT_WINDOW", None)
                    def n(w):
                        return w and w.get("WM_NAME", str(w.__window__())) or "none"
                    print("%s/%s.%s()" % (n(parent), n(win), win.widget["action"]))

                    target = parent or win
                    
                    target["IG_INPUT_ACTION"] = json.dumps(win.widget["action"]).encode("utf-8")
                    display.root.send(display.root,
                                      "IG_INPUT_ACTION", target, "IG_INPUT_ACTION",
                                      event_mask=Xlib.X.SubstructureNotifyMask|Xlib.X.SubstructureRedirectMask)
                    display.flush()
                
        InfiniteGlass.DEBUG("init", "Widgets started\n")

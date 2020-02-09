import InfiniteGlass
import pkg_resources
import json
import numpy
import Xlib.X
import sys

def linestrings2texture(f):
    coastline = json.load(f)

    size = 0
    for feature in coastline["features"]:
        size += len(feature["geometry"]["coordinates"])

    res = []
    for feature in coastline["features"]:
        coords = feature["geometry"]["coordinates"]
        coords = list(zip(coords[:-1], coords[1:]))
        res.extend([z for x in coords for y in x for z in y])

    return res

def setup_shaders(display, shaders = ("DEFAULT", "ROOT", "SPLASH", "SPLASH_BACKGROUND")):
    display.root["IG_SHADER"] = "IG_SHADER_ROOT"
    for SHADER in shaders:
        shader = SHADER.lower()
        for PART in ("GEOMETRY", "VERTEX", "FRAGMENT"):
            part = PART.lower()
            with pkg_resources.resource_stream("glass_theme", "shader_%s_%s.glsl" % (shader, part)) as f:
                display.root["IG_SHADER_%s_%s" % (SHADER, PART)] = f.read()
    display.root["IG_SHADERS"] = ["IG_SHADER_%s" % shader for shader in shaders]

def setup_views(display, views=["IG_VIEW_ROOT", "IG_VIEW_DESKTOP", "IG_VIEW_OVERLAY", "IG_VIEW_MENU"]):
    display.root["IG_VIEW_SPLASH_LAYER"] = "IG_LAYER_SPLASH"
    display.root["IG_VIEW_SPLASH_VIEW"] = [0.0, 0.0, 1.0, 0.0]
    display.root["IG_VIEW_SPLASH_BACKGROUND_LAYER"] = "IG_LAYER_SPLASH_BACKGROUND"
    display.root["IG_VIEW_SPLASH_BACKGROUND_VIEW"] = [0.0, 0.0, 1.0, 0.0]

    display.root["IG_VIEW_MENU_LAYER"] = "IG_LAYER_MENU"
    display.root["IG_VIEW_MENU_VIEW"] = [0.0, 0.0, 1.0, 0.0]
    display.root["IG_VIEW_OVERLAY_LAYER"] = "IG_LAYER_OVERLAY"
    display.root["IG_VIEW_OVERLAY_VIEW"] = [0.0, 0.0, 1.0, 0.0]
    display.root["IG_VIEW_DESKTOP_LAYER"] = "IG_LAYER_DESKTOP"
    display.root["IG_VIEW_DESKTOP_VIEW"] = [0.0, 0.0, 1.0, 0.0]

    display.root["IG_VIEW_ROOT_LAYER"] = "IG_LAYER_ROOT"
    display.root["IG_VIEW_ROOT_VIEW"] = [0.0, 0.0, 1.0, 0.0]

    display.root["IG_VIEWS"] = views

def setup_splash(display):
    w1 = display.root.create_window(map=False)
    w1["IG_LAYER"] = "IG_LAYER_SPLASH"
    w1["IG_SHADER"] = "IG_SHADER_SPLASH"
    w1["IG_DRAW_TYPE"] = "IG_DRAW_TYPE_LINES"
    with pkg_resources.resource_stream("glass_theme", "coastline50.geojson") as f:
        w1["IG_COASTLINE"] = linestrings2texture(f)
    w1.map()

    w2 = display.root.create_window(map=False)
    w2["IG_LAYER"] = "IG_LAYER_SPLASH_BACKGROUND"
    w2["IG_SHADER"] = "IG_SHADER_SPLASH_BACKGROUND"
    w2.map()

    display.root["IG_WORLD_ALPHA"] = 1.

    display.flush()
    
    return w1, w2

def setup_splash_animation(display, lat, lon):
    geom = display.root.get_geometry()
    height = float(geom.height) / float(geom.width)

    display.root["IG_WORLD_LAT"] = lat - 180.
    display.root["IG_WORLD_LON"] = lon - 360.
    display.root["IG_WORLD_ZOOM"] = .1

    display.root["IG_INITIAL_ANIMATE"] = {
        "steps": [
            {"window": display.root, "atom": "IG_VIEW_DESKTOP_VIEW", "dst": [-50.0, height * -50.0, 100.0, height * 100.0]},
            {"window": display.root, "atom": "IG_VIEWS", "dst": ["IG_VIEW_ROOT", "IG_VIEW_DESKTOP", "IG_VIEW_SPLASH_BACKGROUND", "IG_VIEW_SPLASH"]},
            {"tasks": [
                {"window": display.root, "atom": "IG_WORLD_LAT", "timeframe": 3.0, "dst": lat, "easing": "OutCubic"},
                {"window": display.root, "atom": "IG_WORLD_LON", "timeframe": 3.0, "dst": lon, "easing": "OutCubic"},
                {"window": display.root, "atom": "IG_WORLD_ZOOM", "timeframe": 3.0, "dst": 40.0, "easing": "InCubic"}
            ]},
            {"tasks": [
                {"window": display.root, "atom": "IG_WORLD_ALPHA", "timeframe": 2.0, "dst": 0.0},
                {"window": display.root, "atom": "IG_WORLD_ZOOM", "timeframe": 2.0, "dst": 400.0},
                {"window": display.root, "atom": "IG_VIEW_DESKTOP_VIEW", "timeframe": 2.0, "dst": [0.0, 0.0, 1.0, height]}
            ]},
            {"window": display.root, "atom": "IG_VIEWS", "dst": ["IG_VIEW_ROOT", "IG_VIEW_DESKTOP", "IG_VIEW_OVERLAY", "IG_VIEW_MENU"]},
            {"window": display.root, "atom": "IG_THEME", "dst": 1.0}
        ]
    }
    anim = display.root["IG_ANIMATE"]
    anim.send(anim, "IG_ANIMATE", display.root, "IG_INITIAL", 0.0, event_mask=Xlib.X.PropertyChangeMask)

def setup_splash_test(display, lat, lon):
    display.root["IG_VIEWS"] = ["IG_VIEW_SPLASH_BACKGROUND", "IG_VIEW_SPLASH"]
    display.root["IG_WORLD_LAT"] = lat
    display.root["IG_WORLD_LON"] = lon
    display.root["IG_WORLD_ZOOM"] = 1.

    
def main(*arg, **kw):
    with InfiniteGlass.Display() as display:        
        setup_views(display, ["IG_VIEW_SPLASH_BACKGROUND", "IG_VIEW_SPLASH"])
        setup_shaders(display)

        if False:
            display.root["IG_VIEWS"] = ["IG_VIEW_ROOT", "IG_VIEW_DESKTOP", "IG_VIEW_OVERLAY", "IG_VIEW_MENU"]
            return

        splash_windows = setup_splash(display)

        lat = 70.
        lon = 18.
        
        if False:
            setup_splash_test(display, lat, lon)
        else:
            setup_splash_animation(display, lat, lon)
            
        @display.root.on()
        def PropertyNotify(win, event):
            if display.get_atom_name(event.atom) == "IG_THEME":
                for w in splash_windows:
                    w.destroy()
                sys.exit(0)
                
        display.flush()
        
        InfiniteGlass.DEBUG("init", "Theme started\n")

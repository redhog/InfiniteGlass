import InfiniteGlass
import pkg_resources
import json
import numpy
import Xlib.X
import sys
import re
import glass_theme.base

class Theme(glass_theme.base.ThemeBase):
    def __init__(self, *args, **kw):
        glass_theme.base.ThemeBase.__init__(self, *args, **kw)
        
        self.setup_views(self.views)
        self.setup_shaders(self.shaders)

        if False:
            self.display.root["IG_VIEWS"] = ["IG_VIEW_ROOT", "IG_VIEW_DESKTOP", "IG_VIEW_OVERLAY", "IG_VIEW_MENU"]
            return

        lat, lon = self.latlon
        
        if False:
            self.setup_splash_test(lat, lon)
        else:
            self.setup_splash_animation(lat, lon)

        self.display.flush()

    views = ["IG_VIEW_SPLASH_BACKGROUND", "IG_VIEW_SPLASH"]
    shaders = "resource://glass_theme/shaders"
    latlon = (70., 18.)
    
    def linestrings2texture(self, f):
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

    def setup_splash(self):
        w1 = self.display.root.create_window(map=False)
        w1["IG_LAYER"] = "IG_LAYER_SPLASH"
        w1["IG_SHADER"] = "IG_SHADER_SPLASH"
        w1["IG_DRAW_TYPE"] = "IG_DRAW_TYPE_LINES"
        with pkg_resources.resource_stream("glass_theme", "coastline50.geojson") as f:
            w1["IG_COASTLINE"] = self.linestrings2texture(f)
        w1.map()

        w2 = self.display.root.create_window(map=False)
        w2["IG_LAYER"] = "IG_LAYER_SPLASH_BACKGROUND"
        w2["IG_SHADER"] = "IG_SHADER_SPLASH_BACKGROUND"
        w2.map()

        self.display.root["IG_WORLD_ALPHA"] = 1.

        self.display.flush()

        return w1, w2

    def setup_splash_animation(self, lat, lon):
        splash_windows = self.setup_splash()

        geom = self.display.root.get_geometry()
        height = float(geom.height) / float(geom.width)

        self.display.root["IG_WORLD_LAT"] = lat - 180.
        self.display.root["IG_WORLD_LON"] = lon - 360.
        self.display.root["IG_WORLD_ZOOM"] = .1

        self.display.root["IG_INITIAL_ANIMATE"] = {
            "steps": [
                {"window": self.display.root, "atom": "IG_VIEW_DESKTOP_VIEW", "dst": [-50.0, height * -50.0, 100.0, height * 100.0]},
                {"window": self.display.root, "atom": "IG_VIEWS", "dst": ["IG_VIEW_ROOT", "IG_VIEW_DESKTOP", "IG_VIEW_SPLASH_BACKGROUND", "IG_VIEW_SPLASH"]},
                {"tasks": [
                    {"window": self.display.root, "atom": "IG_WORLD_LAT", "timeframe": 3.0, "dst": lat, "easing": "OutCubic"},
                    {"window": self.display.root, "atom": "IG_WORLD_LON", "timeframe": 3.0, "dst": lon, "easing": "OutCubic"},
                    {"window": self.display.root, "atom": "IG_WORLD_ZOOM", "timeframe": 3.0, "dst": 40.0, "easing": "InCubic"}
                ]},
                {"tasks": [
                    {"window": self.display.root, "atom": "IG_WORLD_ALPHA", "timeframe": 2.0, "dst": 0.0},
                    {"window": self.display.root, "atom": "IG_WORLD_ZOOM", "timeframe": 2.0, "dst": 400.0},
                    {"window": self.display.root, "atom": "IG_VIEW_DESKTOP_VIEW", "timeframe": 2.0, "dst": [0.0, 0.0, 1.0, height]}
                ]},
                {"window": self.display.root, "atom": "IG_VIEWS", "dst": ["IG_VIEW_ROOT", "IG_VIEW_DESKTOP", "IG_VIEW_OVERLAY", "IG_VIEW_MENU"]},
                {"window": self.display.root, "atom": "IG_THEME", "dst": 1.0}
            ]
        }
        anim = self.display.root["IG_ANIMATE"]
        anim.send(anim, "IG_ANIMATE", self.display.root, "IG_INITIAL", 0.0, event_mask=Xlib.X.PropertyChangeMask)

        @self.display.root.on()
        def PropertyNotify(win, event):
            if self.display.get_atom_name(event.atom) == "IG_THEME":
                for w in splash_windows:
                    w.destroy()
                sys.exit(0)

    def setup_splash_test(self, lat, lon):
        splash_windows = self.setup_splash()
        self.display.root["IG_VIEWS"] = ["IG_VIEW_SPLASH_BACKGROUND", "IG_VIEW_SPLASH"]
        self.display.root["IG_WORLD_LAT"] = lat
        self.display.root["IG_WORLD_LON"] = lon
        self.display.root["IG_WORLD_ZOOM"] = 1.


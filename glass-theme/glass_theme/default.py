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

        if "IG_THEME" in self.display.root:
            del self.display.root["IG_THEME"]
        
        if self.mode == "no_splash":
            self.display.root["IG_VIEWS"] = ["IG_VIEW_ROOT", "IG_VIEW_DESKTOP", "IG_VIEW_OVERLAY", "IG_VIEW_MENU"]
        elif self.mode == "splash_test":
            self.setup_splash_test()
        else:
            self.setup_splash_animation()

    mode = "splash"

    views = ["IG_VIEW_SPLASH_BACKGROUND", "IG_VIEW_SPLASH"]
    shader_path = "resource://glass_theme/shaders"
    shader_ROOT = "root-fractal-julia"
    latlon = (70., 18.)
    start_latlon = None

    define_EDGE_HINT_WIDTH = 4
    define_EDGE_HINT_COLOR = "vec4(0., 0., 0., 0.3)"
    
    define_margin_left = 6
    define_margin_right = 6
    define_margin_top = 6
    define_margin_bottom = 6

    define_ICON_CUTOFF_1 = .4
    define_ICON_CUTOFF_2 = .3

    define_DECORATION_CUTOFF = 0.25

    define_DECORATION_MOUSEDIST_2 = 150.0
    define_DECORATION_MOUSEDIST_1 = 100.0

    define_BORDER_COLOR_4 = "vec4(0., 0., 0., 1.)"
    define_BORDER_COLOR_1 = "vec4(0., 0., 0., 0.)"

    define_BORDER_ACTIVE_COLOR_1 = "vec4(0., 0., 0., 0.)"
    define_BORDER_ACTIVE_COLOR_2 = "vec4(1., 1., 1., 1.)"
    define_BORDER_ACTIVE_COLOR_3 = "vec4(0., 0., 0., 1.)"
    define_BORDER_ACTIVE_COLOR_4 = "vec4(0., 0., 0., 1.)"

    # See glass-theme/glass_theme/shaders/root/fragment.glsl for list of possible values
    define_BACKGROUND_TYPE = 1
    # 200 gives a much better picture, but is super slow in software...
    define_FRACTAL_PRECISION = 10
    define_BACKGROUND_COLOR_TRANSFORM = ("transpose(mat4(" +
                                         "1.0, 0.0, 0.0, 0.0," +
                                         "0.0, 1.0, 0.0, 0.0," +
                                         "0.0, 0.0, 1.0, 0.0," +
                                         "0.0, 0.0, 0.0, 1.0" +
                                         "))")
    
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
        w1["WM_NAME"] = b"splash"
        w1["IG_LAYER"] = "IG_LAYER_SPLASH"
        w1["IG_SHADER"] = "IG_SHADER_SPLASH"
        w1["IG_DRAW_TYPE"] = "IG_DRAW_TYPE_LINES"
        with pkg_resources.resource_stream("glass_theme", "coastline50.geojson") as f:
            w1["IG_COASTLINE"] = self.linestrings2texture(f)
        w1.map()

        w2 = self.display.root.create_window(map=False)
        w2["WM_NAME"] = b"splash-background"
        w2["IG_LAYER"] = "IG_LAYER_SPLASH_BACKGROUND"
        w2["IG_SHADER"] = "IG_SHADER_SPLASH_BACKGROUND"
        w2.map()

        self.display.root["IG_WORLD_ALPHA"] = 1.

        self.display.flush()

        return w1, w2

    def setup_splash_animation(self):
        lat, lon = self.latlon

        if self.start_latlon is not None:
            start_lat, start_lon = self.start_latlon
        else:
            start_lat = lat - 180.
            start_lon = lon - 360.
            
        splash_windows = self.setup_splash()

        geom = self.display.root.get_geometry()
        height = float(geom.height) / float(geom.width)

        self.display.root["IG_WORLD_LAT"] = start_lat
        self.display.root["IG_WORLD_LON"] = start_lon
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

    def setup_splash_test(self):
        lat, lon = self.latlon
        splash_windows = self.setup_splash()
        self.display.root["IG_VIEWS"] = ["IG_VIEW_SPLASH_BACKGROUND", "IG_VIEW_SPLASH"]
        self.display.root["IG_WORLD_LAT"] = lat
        self.display.root["IG_WORLD_LON"] = lon
        self.display.root["IG_WORLD_ZOOM"] = 1.


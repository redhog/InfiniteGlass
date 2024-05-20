import InfiniteGlass
import json
import Xlib.X
import sys
from . import base
from . import default
from .utils import open_file

class Base(base.ThemeBase):
    theme = default.Theme

class NoSplash(Base):
    def activate(self):
        base.ThemeBase.activate(self)
        self.theme.activate()
        
class BaseSplash(Base):
    latlon = (70., 18.)
    start_latlon = None

    shaders_path = "resource://glass_theme/shaders"
    shader_SPLASH = None
    shader_SPLASH_BACKGROUND = None
    
    root_IG_VIEW_SPLASH_LAYER = "IG_LAYER_SPLASH"
    root_IG_VIEW_SPLASH_VIEW = [0.0, 0.0, 1.0, 0.0]
    root_IG_VIEW_SPLASH_BACKGROUND_LAYER = "IG_LAYER_SPLASH_BACKGROUND"
    root_IG_VIEW_SPLASH_BACKGROUND_VIEW = [0.0, 0.0, 1.0, 0.0]

    root_IG_VIEWS = ["IG_VIEW_SPLASH_BACKGROUND", "IG_VIEW_SPLASH"]
    root_IG_WORLD_ALPHA = 1.

    coastline = "resource://glass_theme/coastline50.geojson"
    
    def activate(self):
        self.theme.activate()
        shaders = self.display.root["IG_SHADERS"]
        base.ThemeBase.activate(self)
        self.display.root["IG_SHADERS"] = shaders + self.display.root["IG_SHADERS"]
        
        if "IG_THEME" in self.display.root:
            del self.display.root["IG_THEME"]

        self.activate_splash()
        
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

    def activate_splash_windows(self):
        w1 = self.display.root.create_window(map=False)
        w1["WM_NAME"] = b"splash"
        w1["IG_LAYER"] = "IG_LAYER_SPLASH"
        w1["IG_SHADER"] = "IG_SHADER_SPLASH"
        w1["IG_DRAW_TYPE"] = "IG_DRAW_TYPE_LINES"
        with open_file(self.coastline) as f:
            w1["IG_COASTLINE"] = self.linestrings2texture(f)
        w1.map()

        w2 = self.display.root.create_window(map=False)
        w2["WM_NAME"] = b"splash-background"
        w2["IG_LAYER"] = "IG_LAYER_SPLASH_BACKGROUND"
        w2["IG_SHADER"] = "IG_SHADER_SPLASH_BACKGROUND"
        w2.map()

        self.display.flush()

        return w1, w2

class SplashAnimation(BaseSplash):
    root_IG_WORLD_ZOOM = 0.1
    
    def activate_splash(self):
        lat, lon = self.latlon

        if self.start_latlon is not None:
            start_lat, start_lon = self.start_latlon
        else:
            start_lat = lat - 180.
            start_lon = lon - 360.

        splash_windows = self.activate_splash_windows()

        geom = self.display.root.get_geometry()
        height = float(geom.height) / float(geom.width)

        self.display.root["IG_WORLD_LAT"] = start_lat
        self.display.root["IG_WORLD_LON"] = start_lon

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
                {"window": self.display.root, "atom": "IG_VIEWS", "dst": self.theme.root_IG_VIEWS},
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

                self.theme.activate()
                
                #sys.exit(0)

class SplashTest(BaseSplash):
    root_IG_WORLD_ZOOM = 1.0
    
    def activate_splash(self):
        lat, lon = self.latlon
        splash_windows = self.activate_splash_windows()
        self.display.root["IG_WORLD_LAT"] = lat
        self.display.root["IG_WORLD_LON"] = lon

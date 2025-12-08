import InfiniteGlass
import InfiniteGlass.action
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
    def __init__(self, display, **kw):
        Base. __init__(self, display, **kw)
        self.theme.root_IG_VIEWS_ORIG = self.theme.root_IG_VIEWS
        self.theme.root_IG_VIEW_DESKTOP_VIEW_ORIG = self.theme.root_IG_VIEW_DESKTOP_VIEW
        self.theme.root_IG_VIEWS = None
        self.theme.root_IG_VIEW_DESKTOP_VIEW = None
        
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
        theme_shaders = self.display.root["IG_SHADERS"]
        base.ThemeBase.activate(self)
        splash_shaders = self.display.root["IG_SHADERS"]
        self.display.root["IG_SHADERS"] = theme_shaders + splash_shaders
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
        lat, lon = self.latlon

        if self.start_latlon is not None:
            start_lat, start_lon = self.start_latlon
        else:
            start_lat = lat - 180.
            start_lon = lon - 360.
        
        w1 = self.display.root.create_window(map=False)
        w1["WM_NAME"] = b"splash"
        w1["IG_LAYER"] = "IG_LAYER_SPLASH"
        w1["IG_SHADER"] = "IG_SHADER_SPLASH"
        w1["IG_DRAW_TYPE"] = "IG_DRAW_TYPE_LINES"
        with open_file(self.coastline) as f:
            w1["IG_COASTLINE"] = self.linestrings2texture(f)
        w1["IG_WORLD_LATLONS"] = [start_lat, start_lon, lat, lon]
            
        w1.map()

        w2 = self.display.root.create_window(map=False)
        w2["WM_NAME"] = b"splash-background"
        w2["IG_LAYER"] = "IG_LAYER_SPLASH_BACKGROUND"
        w2["IG_SHADER"] = "IG_SHADER_SPLASH_BACKGROUND"
        w2.map()
        self.display.root["IG_SPLASH_WINDOWS"] = [w1, w2]
        
        self.display.flush()

        return w1, w2

class SplashAnimation(BaseSplash):
    root_IG_WORLD_ZOOM = 0.1
    
    def activate_splash(self):
        self.splash_windows = self.activate_splash_windows()
        InfiniteGlass.action.ActionRunner(self.display).run({
            "splash_zoom_in": {}})

class SplashTest(BaseSplash):
    root_IG_WORLD_ZOOM = 1.0
    
    def activate_splash(self):
        lat, lon = self.latlon
        splash_windows = self.activate_splash_windows()
        self.display.root["IG_WORLD_LAT"] = lat
        self.display.root["IG_WORLD_LON"] = lon

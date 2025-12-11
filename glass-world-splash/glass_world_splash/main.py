import InfiniteGlass
import Xlib
import json

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


def create_splash_windows(display, coastline="resource://glass_theme/coastline50.geojson"):
    w1 = display.root.create_window(map=False)
    w1["WM_NAME"] = b"splash"
    w1["IG_LAYER"] = "IG_LAYER_NONE"
    w1["IG_SHADER"] = "IG_SHADER_SPLASH"
    w1["IG_DRAW_TYPE"] = "IG_DRAW_TYPE_LINES"
    with InfiniteGlass.utils.open_file(coastline) as f:
        w1["IG_COASTLINE"] = linestrings2texture(f)
    w1.map()

    w2 = display.root.create_window(map=False)
    w2["WM_NAME"] = b"splash-background"
    w2["IG_LAYER"] = "IG_LAYER_NONE"
    w2["IG_SHADER"] = "IG_SHADER_SPLASH_BACKGROUND"
    w2.map()

    display.root["IG_WORLD_SPLASH_WINDOWS"] = [w1, w2]
    
    return w1, w2

def splash_unlock(self, timeframe = 5.0, **kw):
    latlon = self.display.root.get("IG_WORLD_COORDS", [70., 18.])
    if len(latlon) == 2:
        latlon = [latlon[0]-180, latlon[1]-360] + latlon
        
    start_lat, start_lon, lat, lon = latlon

    w1, w2 = self.display.root["IG_WORLD_SPLASH_WINDOWS"]

    geom = self.display.root.get_geometry()
    height = float(geom.height) / float(geom.width)

    view_orig = self.display.root.get("IG_VIEW_DESKTOP_VIEW_ORIG", [0.0, 0.0, 1.0, height])
    
    timeframe = timeframe / 5. # Sum of all timeframes
    self.display.root["IG_INITIAL_ANIMATE"] = {
        "steps": [
            {"window": self.display.root, "atom": "IG_WORLD_ZOOM", "dst": 0.1},
            {"window": self.display.root, "atom": "IG_WORLD_LAT", "dst": start_lat},
            {"window": self.display.root, "atom": "IG_WORLD_LON", "dst": start_lon},
            {"window": self.display.root, "atom": "IG_WORLD_ALPHA", "dst": 1.0},
            {"window": w1, "atom": "IG_LAYER", "dst": "IG_LAYER_SPLASH"},
            {"window": w2, "atom": "IG_LAYER", "dst": "IG_LAYER_SPLASH_BACKGROUND"},
            {"window": self.display.root, "atom": "IG_VIEW_DESKTOP_VIEW", "dst": [-50.0, height * -50.0, 100.0, height * 100.0]},
            {"tasks": [
                {"window": self.display.root, "atom": "IG_WORLD_LAT", "timeframe": timeframe * 3.0, "dst": lat, "easing": "OutCubic"},
                {"window": self.display.root, "atom": "IG_WORLD_LON", "timeframe": timeframe * 3.0, "dst": lon, "easing": "OutCubic"},
                {"window": self.display.root, "atom": "IG_WORLD_ZOOM", "timeframe": timeframe * 3.0, "dst": 40.0, "easing": "InCubic"}
            ]},
            {"tasks": [
                {"window": self.display.root, "atom": "IG_WORLD_ALPHA", "timeframe": timeframe * 2.0, "dst": 0.0},
                {"window": self.display.root, "atom": "IG_WORLD_ZOOM", "timeframe": timeframe * 2.0, "dst": 400.0},
                {"window": self.display.root, "atom": "IG_VIEW_DESKTOP_VIEW", "timeframe": timeframe * 2.0, "dst": view_orig}
            ]},
            {"window": w1, "atom": "IG_LAYER", "dst": "IG_LAYER_NONE"},
            {"window": w2, "atom": "IG_LAYER", "dst": "IG_LAYER_NONE"},
        ]
    }
    anim = self.display.root["IG_ANIMATE"]
    anim.send(anim, "IG_ANIMATE", self.display.root, "IG_INITIAL", 0.0, event_mask=Xlib.X.PropertyChangeMask)
    

def splash_lock(self, timeframe=5.0, **kw):
    latlon = self.display.root.get("IG_WORLD_COORDS", [70., 18.])
    if len(latlon) == 2:
        latlon = [latlon[0]-180, latlon[1]-360] + latlon
        
    start_lat, start_lon, lat, lon = latlon

    w1, w2 = self.display.root["IG_WORLD_SPLASH_WINDOWS"]

    geom = self.display.root.get_geometry()
    height = float(geom.height) / float(geom.width)

    self.display.root["IG_VIEW_DESKTOP_VIEW_ORIG"] = self.display.root["IG_VIEW_DESKTOP_VIEW"]
    
    timeframe = timeframe / 5. # Sum of all timeframes
    self.display.root["IG_INITIAL_ANIMATE"] = {
        "steps": [
            {"window": self.display.root, "atom": "IG_WORLD_ZOOM", "dst": 400.0},
            {"window": self.display.root, "atom": "IG_WORLD_LAT", "dst": lat},
            {"window": self.display.root, "atom": "IG_WORLD_LON", "dst": lon},
            {"window": self.display.root, "atom": "IG_WORLD_ALPHA", "dst": 0.0},
            {"window": w1, "atom": "IG_LAYER", "dst": "IG_LAYER_SPLASH"},
            {"window": w2, "atom": "IG_LAYER", "dst": "IG_LAYER_SPLASH_BACKGROUND"},
            {"tasks": [
                {"window": self.display.root, "atom": "IG_WORLD_ALPHA", "timeframe": timeframe * 2.0, "dst": 1.0},
                {"window": self.display.root, "atom": "IG_WORLD_ZOOM", "timeframe": timeframe * 2.0, "dst": 40.0},
                {"window": self.display.root, "atom": "IG_VIEW_DESKTOP_VIEW", "timeframe": timeframe * 2.0, "dst": [-50.0, height * -50.0, 100.0, height * 100.0]}
            ]},
            {"tasks": [
                {"window": self.display.root, "atom": "IG_WORLD_LAT", "timeframe": timeframe * 3.0, "dst": start_lat, "easing": "InCubic"},
                {"window": self.display.root, "atom": "IG_WORLD_LON", "timeframe": timeframe * 3.0, "dst": start_lon, "easing": "InCubic"},
                {"window": self.display.root, "atom": "IG_WORLD_ZOOM", "timeframe": timeframe * 3.0, "dst": 0.1, "easing": "OutCubic"}
            ]},
        ]
    }
    anim = self.display.root["IG_ANIMATE"]
    anim.send(anim, "IG_ANIMATE", self.display.root, "IG_INITIAL", 0.0, event_mask=Xlib.X.PropertyChangeMask)
    

@InfiniteGlass.profilable
def main(*arg, **kw):
    with InfiniteGlass.Display() as display:
        w1, w2 = create_splash_windows(display)
        @w1.on()
        def KeyPress(win, event):
            pass
        

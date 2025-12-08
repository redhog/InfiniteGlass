import Xlib
import InfiniteGlass

def splash_zoom_in(self, **kw):
    w1, w2 = splash_windows = self.display.root["IG_SPLASH_WINDOWS"]
    [start_lat, start_lon, lat, lon] = w1["IG_WORLD_LATLONS"]

    geom = self.display.root.get_geometry()
    height = float(geom.height) / float(geom.width)

    self.display.root["IG_WORLD_LAT"] = start_lat
    self.display.root["IG_WORLD_LON"] = start_lon

    self.display.root["IG_VIEWS"] = ["IG_VIEW_SPLASH_BACKGROUND", "IG_VIEW_SPLASH"]
    
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
            {"window": self.display.root, "atom": "IG_VIEWS", "dst": self.display.root["IG_VIEWS_ORIG"]},
#            {"window": self.display.root, "atom": "IG_VIEWS", "dst": self.display.root["IG_VIEWS_ORIG"]},
#            {"window": self.display.root, "atom": "IG_THEME", "dst": 1.0}
        ]
    }
    anim = self.display.root["IG_ANIMATE"]
    anim.send(anim, "IG_ANIMATE", self.display.root, "IG_INITIAL", 0.0, event_mask=Xlib.X.PropertyChangeMask)
    self.display.flush()

    @self.display.root.on()
    def PropertyNotify(win, event):
        if self.display.get_atom_name(event.atom) == "IG_THEME":
            for w in [w1, w2]:
                w.unmap()


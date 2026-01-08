import InfiniteGlass
import numpy
from . import mode
from . import utils
import Xlib.X

def write(self, *args, **kw):
    return self.write(*args, **kw)

class TitleSearchMode(mode.Mode):        
    def query(self, query=""):
        for win in self.windows:
            if query in win["name"]:
                yield win
     
    def bbox(self, query=""):
        match = list(self.query(query))
        print("Matching windows:", ",".join(w["name"] for w in match))
        if not match:
            return None
        return utils.bbox([win["coords"] for win in match])

    def bbox_view(self, query=""):
        res = self.bbox(query)
        if not res: return self.orig_view
        return utils.bbox_view(res, self.orig_view)
        
    def enter(self):
        mode.Mode.enter(self)

        self.orig_view = self.display.root["IG_VIEW_DESKTOP_VIEW"]
        self.aspect_ratio = self.orig_view[2] / self.orig_view[3]

        self.size = self.display.root["IG_VIEW_DESKTOP_SIZE"]
        self.windows = list(self.get_all_windows())
        print("All windows:", ",".join(w["name"] for w in self.windows))

        self.input = ""

        self.label_window = self.display.root.create_window(map=False)
        self.label_window["WM_NAME"] = b"BBOX label"
        self.label_window["_NET_WM_WINDOW_TYPE"] = "_NET_WM_WINDOW_TYPE_DESKTOP"
        self.display.root["IG_VIEW_DESKTOP_VIEW"] = self.bbox_view()
        self.generate_overlay()
        self.label_window.map()

        self.orig_menu_view = self.display.root["IG_VIEW_MENU_VIEW"]
        self.input_window = self.display.root.create_window(map=False)
        self.input_window["WM_NAME"] = b"Title search"
        self.input_window["_NET_WM_WINDOW_TYPE"] = "_NET_WM_WINDOW_TYPE_DESKTOP"

        self.update_input()
        self.input_window["IG_SIZE"] = [500, 500]

        self.input_window["IG_COORDS"] = [self.orig_menu_view[0], self.orig_menu_view[1] + self.orig_menu_view[3], self.orig_menu_view[2], self.orig_menu_view[3]]
        self.input_window["IG_LAYER"] = "IG_LAYER_MENU"

        self.input_window.map()

        self.display.flush()
            
        return True

    def exit(self):
        self.label_window.destroy()
        self.input_window.destroy()

    def write(self, event):
        if (event.state & ~Xlib.X.ShiftMask & ~Xlib.X.Mod1Mask) != 0:
            return
        keycode = self.display.keycode_to_keysym(event.detail, event.state)
        s = self.display.lookup_string(keycode)
        if s is not None:
            if s == "\b":
                self.input = self.input[:-1]
            else:
                self.input += s
            self.update_input()

        self.display.flush()
        
        bbox = self.bbox_view(self.input)
        if bbox is not None:
            self.display.root["IG_VIEW_DESKTOP_VIEW"] = bbox

        self.display.flush()
            
    def update_input(self):
        
        w = self.size[0]
        h = self.size[1]
        bw = w * 0.5
        bh = h * 0.1
        bx = (w - bw) / 2
        by = (h - bh) / 2
        th = h * 0.10
        tx = w * 0.02
        ty = th * 0.8
        by2 = by - 3
        bx2 = bx - 3
        bw2 = bw + 6
        bh2 = bh + 6
        
        svg = """<?xml version="1.0" encoding="UTF-8" standalone="no"?>
                  <svg
                    xmlns="http://www.w3.org/2000/svg"
                    id="svg8"
                    version="1.1"
                    viewBox="0 0 %(w)s %(h)s"
                    width="%(w)smm"
                    height="%(h)smm">
                    <rect x="%(bx2)s" y="%(by2)s" width="%(bw2)s" height="%(bh2)s" style="fill-opacity:0;stroke-width:2;stroke:#000000;" />
                    <rect x="%(bx)s" y="%(by)s" width="%(bw)s" height="%(bh)s" style="fill:#ffffff;stroke-width:1;stroke:#000000;" />
                    <svg x="%(bx)s" y="%(by)s" width="%(bw)s" height="%(bh)s">
                      <text x="%(tx)s" y="%(ty)s" style="font-size: %(th)s; fill:#000000;">%(content)s|</text>
                    </svg>
                  </svg>""" % {
                      "w": w, "h": h,
                      "bw": bw, "bh": bh, "bx": bx, "by": by,
                      "bw2": bw2, "bh2": bh2, "bx2": bx2, "by2": by2, 
                      "th": th, "tx": tx, "ty": ty,
                      "content": self.input}

        self.input_window["IG_CONTENT"] = ("IG_SVG", svg.encode("UTF-8"))
        
    def generate_overlay(self):
        bbox = self.bbox()

        if not bbox:
            bbox = [self.orig_view[0],
                    self.orig_view[1] - self.orig_view[3],
                    self.orig_view[2],
                    self.orig_view[3]]
        
        bbox[0] -= bbox[2] * 0.1
        bbox[1] += bbox[3] * 0.1
        bbox[2] *= 1.2
        bbox[3] *= 1.2

        self.label_window["IG_COORDS"] = bbox
        
        window_svg = [
            """
              <rect x="%(x)s" y="%(y)s" width="%(w)s" height="%(h)s" style="fill: #ffffff; opacity: 1.0;" />
              <svg x="%(x)s" y="%(y)s" width="%(w)s" height="%(h)s">
                <text
                  xml:space="preserve"
                  style="font-size:%(fontsize)spx;font-family:'Times New Roman';fill:#000000;"
                  x="%(tx)s"
                  y="%(ty)s">%(name)s</text>
              </svg>

            """ % {
                "fontsize": window["coords"][3] * 0.1,
                "x": window["coords"][0],
                "y": -window["coords"][1],
                "tx": window["coords"][2] * 0.05, 
                "ty": window["coords"][3] * 0.1, 
                "w": window["coords"][2],
                "h": window["coords"][3] * 0.15,
                "name": window["name"].strip()
            } for window in self.windows
        ]
        
        svg = """<?xml version="1.0" encoding="UTF-8" standalone="no"?>
              <svg
                xmlns="http://www.w3.org/2000/svg"
                id="svg8"
                version="1.1"
                viewBox="%(x)s %(y)s %(width)s %(height)s"
                width="%(widthx)smm"
                height="%(heightx)smm">
                <!-- rect x="%(x)s" y="%(y)s" width="%(width)s" height="%(height)s" fill="#0000ff" style="opacity: 0.5;" / -->
                %(content)s
              </svg>""" % {
                  "widthx": 1000.,
                  "heightx": 1000. * bbox[3] / bbox[2],
                  "width": bbox[2],
                  "height": bbox[3],
                  "x": bbox[0],
                  "y": -bbox[1],
                  "content": "\n".join(window_svg)}

        self.label_window["IG_CONTENT"] = ("IG_SVG", svg.encode("UTF-8"))

    def get_all_windows(self):
        for child in self.display.root.query_tree().children:
            if child.get_attributes().map_state != Xlib.X.IsViewable: continue
            coords = child.get("IG_COORDS", None)
            if coords is None: continue
            name = child.get("WM_NAME", None)
            if name is None: continue
            cls = child.get("WM_CLASS", None)
            if cls is None: continue
            layer = child.get("IG_LAYER", "IG_LAYER_DESKTOP")
            if layer != "IG_LAYER_DESKTOP": continue
            yield {
                "window": child,
                "coords": coords,
                "name": name.decode("utf-8"),
                "class": [c.decode("utf-8") for c in cls]}

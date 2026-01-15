import InfiniteGlass
import numpy
from . import mode
from . import utils
import Xlib.X

def write(self, *args, **kw):
    return self.write(*args, **kw)

class TitleSearchMode(mode.Mode):
    svg = """<?xml version="1.0" encoding="UTF-8" standalone="no"?>
  <svg
    xmlns="http://www.w3.org/2000/svg"
    xmlns:xlink="http://www.w3.org/1999/xlink"
    id="svg8"
    version="1.1"
    viewBox="0 0 %(w)s %(h)s"
    width="%(w)smm"
    height="%(h)smm">
    <rect x="%(bx2)s" y="%(by2)s" width="%(bw2)s" height="%(bh2)s" style="fill-opacity:0;stroke-width:2;stroke:#000000;" />
    <rect x="%(bx)s" y="%(by)s" width="%(bw)s" height="%(bh)s" style="fill:#ffffff;stroke-width:1;stroke:#000000;" />
    <svg x="%(bx)s" y="%(by)s" width="%(bw)s" height="%(bh)s">
      <text x="%(tx)s" y="%(ty)s" style="font-size: %(th)s; fill:#000000;" xml:space="preserve"><use xlink:href="property://IG_INPUT" />|</text>
    </svg>
</svg>"""
    
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
        self.label_window["IG_SIZE"] = self.size
        self.display.root["IG_VIEW_DESKTOP_VIEW"] = self.bbox_view()
        self.generate_overlay()
        self.label_window.map()

        self.orig_menu_view = self.display.root["IG_VIEW_MENU_VIEW"]
        self.input_window = self.display.root.create_window(map=False)
        self.input_window["WM_NAME"] = b"Title search"
        self.input_window["_NET_WM_WINDOW_TYPE"] = "_NET_WM_WINDOW_TYPE_DESKTOP"
        self.input_window["IG_COORDS"] = [self.orig_menu_view[0], self.orig_menu_view[1] + self.orig_menu_view[3], self.orig_menu_view[2], self.orig_menu_view[3]]
        self.input_window["IG_SIZE"] = self.size
        self.input_window["IG_LAYER"] = "IG_LAYER_MENU"
        
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
        
        svg = self.svg % {
            "w": w, "h": h,
            "bw": bw, "bh": bh, "bx": bx, "by": by,
            "bw2": bw2, "bh2": bh2, "bx2": bx2, "by2": by2, 
            "th": th, "tx": tx, "ty": ty}

        self.input_window["IG_CONTENT"] = ("IG_SVG", svg.encode("UTF-8"))
        self.input_window["IG_INPUT"] = self.input.encode("UTF-8")

        self.input_window.map()

        self.display.flush()
            
        return True

    def exit(self):
        self.label_window.destroy()
        self.input_window.destroy()

    def write(self, event):
        index = 1 if event.state & Xlib.X.ShiftMask else 0
        keycode = self.display.keycode_to_keysym(event.detail, index)
        s = self.display.lookup_string(keycode)
        if s is not None:
            if s == "\b":
                self.input = self.input[:-1]
            else:
                self.input += s
            self.input_window["IG_INPUT"] = self.input.encode("UTF-8")
            
        self.display.flush()
        
        bbox = self.bbox_view(self.input)
        if bbox is not None:
            self.display.root["IG_VIEW_DESKTOP_VIEW"] = bbox

        self.generate_overlay()
            
        self.display.flush()

    def generate_overlay(self):
        view = self.bbox_view(self.input)

        if not view:
            view = [
                self.orig_view[0],
                self.orig_view[1],
                self.orig_view[2],
                self.orig_view[3],
            ]

        bbox = [view[0], view[1] + view[3], view[2], view[3]]

        self.label_window["IG_COORDS"] = bbox

        scale = 1000. / bbox[2]

        bbox[0] *= scale
        bbox[1] *= scale
        bbox[2] *= scale
        bbox[3] *= scale
        
        window_elements = []

        for window in self.query(self.input):
            left, top, width, height = window["coords"]
            left *= scale
            top *= scale
            width *= scale
            height *= scale

            # window-space geometry
            rect_w = width
            rect_h = height * 0.18
            rect_x = left
            rect_y = top

            font_size = height * 0.12
            text_x = left + width * 0.05
            text_y = top - font_size

            window_elements.append(
                f"""
                <rect
                  x="{rect_x}"
                  y="{-rect_y}"
                  width="{rect_w}"
                  height="{rect_h}"
                  fill="#ffffff"
                  opacity="1.0"/>

                <text
                  x="{text_x}"
                  y="{-text_y}"
                  font-size="{font_size}"
                  font-family="serif"
                  fill="#000000">
                  {window["name"].strip()}
                </text>
                """
            )

        svg = f"""<?xml version="1.0" encoding="UTF-8"?>
    <svg
      xmlns="http://www.w3.org/2000/svg"
      viewBox="{bbox[0]} {-bbox[1]} {bbox[2]} {bbox[3]}"
      preserveAspectRatio="none">

      <!-- rect
       x="{bbox[0]}"
       y="{-bbox[1]}"
       width="{bbox[2]}"
       height="{bbox[3]}"
       fill="#0000ff"
       opacity="0.5"/ -->
      {''.join(window_elements)}

    </svg>
    """

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

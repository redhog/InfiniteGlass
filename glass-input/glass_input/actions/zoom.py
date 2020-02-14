import InfiniteGlass
import math
from .. import mode

def zoom(self, factor, around_aspect=(0.5, 0.5), around_pos=None, view="IG_VIEW_DESKTOP_VIEW"):
    "Zoom the screen in or out"
    screen = list(self.display.root[view])
    if around_pos is None:
        around_pos = (screen[0] + screen[2] * around_aspect[0],
                      screen[1] + screen[3] * around_aspect[1])
    else:
        around_aspect = ((around_pos[0] - screen[0]) / screen[2],
                         (around_pos[1] - screen[1]) / screen[3])
    screen[2] *= factor
    screen[3] *= factor
    screen[0] = around_pos[0] - screen[2] * around_aspect[0]
    screen[1] = around_pos[1] - screen[3] * around_aspect[1]
    self.display.root[view] = screen

def zoom_in(self, event):
    "Zoom the screen in one step"
    print("ZOOM IN")
    zoom(self, 1 / 1.1)

def zoom_out(self, event):
    "Zoom the screen out one step"
    print("ZOOM OUT")
    zoom(self, 1.1)

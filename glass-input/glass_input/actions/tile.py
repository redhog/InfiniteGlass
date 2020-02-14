from .. import mode
import InfiniteGlass
import numpy as np
import rpack
from . import item_zoom_to

def tile_visible(self, event, grain=2, margins=0.02, zoom_1_1=False):
    "Tile/pack all visible windows as tightly as possible"

    view = list(self.display.root["IG_VIEW_DESKTOP_VIEW"])
    
    visible, overlap, invisible = InfiniteGlass.windows.get_windows(self.display, view)
    windows = visible+overlap

    sizes = np.array([coords[2:] for window, coords in windows])
    sizes += margins * view[2]
    
    scaling = 10**(-np.round(np.log10(sizes.min())) + grain)
    sizes = np.array(scaling * sizes, dtype=int)
    positions = np.array(rpack.pack([[int(y) for y in x] for x in sizes]))
    maxpos = (positions + sizes).max(axis=0)

    if maxpos[0] / maxpos[1] > view[2] / view[3]:
        scaling = view[2] / maxpos[0]
    else:
        scaling = view[3] / maxpos[1]
        
    positions = positions * scaling
    sizes = sizes * scaling
        
    positions[:,0] += view[0]
    positions[:,1] += view[1]
    positions[:,1] += sizes[:,1]

    sizes -= margins * view[2]
    positions[:,0] += margins * view[2] / 2
    positions[:,1] -= margins * view[2] / 2
    
    for (window, coords), position, size in zip(windows, positions, sizes):
        new_coords = [position[0], position[1], size[0], size[1]]
        window["IG_COORDS_ANIMATE"] = new_coords
        if zoom_1_1:
            window["IG_SIZE"] = item_zoom_to.item_zoom_1_1_to_sreen_calc(self, window, coords = new_coords)
        else:
            pxsize = window["IG_SIZE"]
            pxsize[1] = int(pxsize[0] * size[1] / size[0])
            window["IG_SIZE"] = pxsize

        self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", window, "IG_COORDS", .5)
            
    self.display.flush()

def tile_visible_to_1_1(self, *arg, **kw):
    "Tile/pack all visible windows as tightly as possible and change their pixel resolution to match their size on the screen"
    tile_visible(self, zoom_1_1=True, *arg, **kw)

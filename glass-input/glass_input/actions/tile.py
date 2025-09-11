import InfiniteGlass
import numpy as np
from . import item_zoom_to
import importlib

def load_fn(name):
    mod, fn = name.rsplit(".", 1)
    return getattr(importlib.import_module(mod), fn)

def tile_visible(self, event, grain=2, margins=0.02, zoom_1_1=False, packer="glass_input.binary_tree_bin_packer.GrowingPacker"):
    "Tile/pack all visible windows as tightly as possible"

    view = list(self.display.root["IG_VIEW_DESKTOP_VIEW"])
    
    visible, overlap, invisible = InfiniteGlass.windows.get_windows(self.display, view)
    windows = visible+overlap
    
    packer = load_fn(packer)()
    blocks = [{"w": coords[2] + margins * view[2], "h": coords[3] + margins * view[2], "window": window} for window, coords in windows]
    packer.fit(blocks, view[2], view[3], sorting="diagonal")
    
    positions = np.array([(block["fit"]["x"], block["fit"]["y"], block["w"], block["h"]) for block in blocks])
    
    minpos = (positions[:,:2]).min(axis=0)
    positions[:,0] -= minpos[0]
    positions[:,1] -= minpos[1]
    maxpos = (positions[:,:2] + positions[:,2:]).max(axis=0)

    if maxpos[0] / maxpos[1] > view[2] / view[3]:
        scaling = view[2] / maxpos[0]
    else:
        scaling = view[3] / maxpos[1]
        
    positions = positions * scaling

    positions[:,1] += positions[:,3]

    positions[:,0] += view[0] + (view[2] - ((positions[:,0] + positions[:,2]).max() - positions[:,0].min())) / 2
    positions[:,1] += view[1] + (view[3] - (positions[:,1].max() - (positions[:,1] - positions[:,3]).min())) / 2

    positions[:,2:] -= margins * view[2]
    positions[:,0] += margins * view[2] / 2
    positions[:,1] -= margins * view[2] / 2
    
    for block, new_coords in zip(blocks, positions):
        window = block["window"]
        new_coords = list(new_coords)
        window["IG_COORDS_ANIMATE"] = new_coords
        if zoom_1_1:
            window["IG_SIZE"] = item_zoom_to.item_zoom_1_1_to_sreen_calc(self, window, coords = new_coords)
        else:
            pxsize = window["IG_SIZE"]
            pxsize[1] = int(pxsize[0] * new_coords[3] / new_coords[2])
            window["IG_SIZE"] = pxsize

        self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", window, "IG_COORDS", .5)
            
    self.display.flush()

def tile_visible_to_1_1(self, *arg, **kw):
    "Tile/pack all visible windows as tightly as possible and change their pixel resolution to match their size on the screen"
    tile_visible(self, zoom_1_1=True, *arg, **kw)

import numpy as np
import coordtest

def item_type_widget_get_tile(item_coords, snap = 1):
    itempixelwidth = item_coords[2] * coordtest.pixels[0] / coordtest.screen[2]
    itempixelheight = item_coords[3] * coordtest.pixels[1] / coordtest.screen[3]

    # x and y are ]0,1[, from top left to bottom right of window.
    x1 = (coordtest.screen[0] - item_coords[0]) / item_coords[2]
    y1 = (item_coords[1] - (coordtest.screen[1] + coordtest.screen[3])) / item_coords[3]

    x2 = (coordtest.screen[0] + coordtest.screen[2] - item_coords[0]) / item_coords[2]
    y2 = (item_coords[1] - coordtest.screen[1]) / item_coords[3]

    if x1 < 0.: x1 = 0.
    if x1 > 1.: x1 = 1.
    if y1 < 0.: y1 = 0.
    if y1 > 1.: y1 = 1.
    if x2 < 0.: x2 = 0.
    if x2 > 1.: x2 = 1.
    if y2 < 0.: y2 = 0.
    if y2 > 1.: y2 = 1.

    # When screen to window is 1:1 this holds:
    # item_coords[2] = item->width * coordtest.screen[2] / coordtest.pixels[0];
    # item_coords[3] = item->height * coordtest.screen[3] / coordtest.pixels[1];

    px1 = int(x1 * float(itempixelwidth))
    px2 = int(x2 * float(itempixelwidth))
    py1 = int(y1 * float(itempixelheight))
    py2 = int(y2 * float(itempixelheight))

    if snap > 1:
        px1 -= px1 % snap
        px2 -= px2 % snap
        py1 -= py1 % snap
        py2 -= py2 % snap
    
    pixelwidth = px2 - px1
    pixelheight = py2 - py1
    
    return {
        "width": pixelwidth,
        "height": pixelheight,
        "x": px1,
        "y": py1,
        "itemwidth": itempixelwidth,
        "itemheight": itempixelheight}

def shader_widget_fragment(transform, window_coord):
    transform_mat = np.array([
        [1./transform[2], 0., 0., -transform[0]/transform[2]],
        [0., 1./transform[3], 0., -transform[1]/transform[3]],
        [0., 0., 1., 0.],
        [0., 0., 0., 1.]])
    return transform_mat.dot(np.array([[window_coord[0]], [window_coord[1]], [0], [1.]]))

def item_type_widget_draw(item_coords):
    tile = item_type_widget_get_tile(item_coords, snap=100)
    idealtile = item_type_widget_get_tile(item_coords)

    transform = [idealtile["x"] - tile["x"],
                 idealtile["y"] - tile["y"],
                 idealtile["itemwidth"] / tile["itemwidth"],
                 idealtile["itemheight"] / tile["itemheight"]]

    transform = [0, 0, 1, 1]
    
    for windowy in (0, 1):
        for windowx in (0, 1):
            coords = shader_widget_fragment(transform, [windowx, windowy])
            print("%s,%s => %s,%s" % (windowx, windowy, coords[0][0], coords[1][0]))

print coordtest.screen, coordtest.pixels
item_type_widget_draw([0,1,1,.75])

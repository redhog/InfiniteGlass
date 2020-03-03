import InfiniteGlass.windows

def bbox(window_coords=None):
    xs = [x for coords in window_coords for x in (coords[0], coords[0] + coords[2])]
    ys = [y for coords in window_coords for y in (coords[1], coords[1] - coords[3])]
    return [min(*xs), max(*ys), max(*xs) - min(*xs), max(*ys) - min(*ys)]

def bbox_view(self, bbox, orig_view):
    aspect_ratio = orig_view[2] / orig_view[3]
    bbox = list(bbox)
    
    h = bbox[2] / aspect_ratio
    w = bbox[3] * aspect_ratio
    if h > bbox[3]:
        bbox[3] = h
    else:
        bbox[2] = w
    bbox[1] -= bbox[3]
    return bbox

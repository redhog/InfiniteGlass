import Xlib.X
import Xlib.error
from . import coords as coordsmod 
from . import debug

def get_view_layers(display):
    def tuplify(value):
        if isinstance(value, (list, tuple)):
            return value
        return (value,)
    return [(layer, display.root[name + "_VIEW"], display.root[name + "_SIZE"])
            for name in display.root["IG_VIEWS"]
            for layer in tuplify(display.root[name + "_LAYER"])]

def is_inside_window(display, window):
    pointer = display.root.query_pointer()

    try:
        views = get_view_layers(display)
    except KeyError:
        return False
    spacecoords = {layer: coordsmod.view_to_space(view, size, pointer.root_x, pointer.root_y)
                   for layer, view, size in views}
    try:
        if window.get_attributes().map_state != Xlib.X.IsViewable: return False
        coords = window.get("IG_COORDS", None)
        if coords is None: return False
        layer = window.get("IG_LAYER", "IG_LAYER_DESKTOP")
        if layer not in spacecoords: return False
        pointer = spacecoords[layer]
        if (pointer[0] >= coords[0]
            and pointer[0] <= coords[0] + coords[2]
            and pointer[1] <= coords[1]
            and pointer[1] >= coords[1] - coords[3]):
            return window
    except Xlib.error.BadWindow as e:
        debug.DEBUG("is_inside_window", "%s: %s" % (window.__window__(), e))
    return False

def get_active_window(display):
    try:
        views = get_view_layers(display)
    except KeyError:
        return None

    pointer = display.root.query_pointer()
    windows = [(win, win.get("IG_LAYER", "IG_LAYER_DESKTOP"), win["IG_COORDS"])
                for win in display.root.query_tree().children
                if "IG_COORDS" in win and win.get_attributes().map_state == Xlib.X.IsViewable]
    
    for layer, view, size in reversed(views):
        space_pointer = coordsmod.view_to_space(view, size, pointer.root_x, pointer.root_y)
        for win, win_layer, coords in windows:
            try:
                if (layer == win_layer
                    and space_pointer[0] >= coords[0]
                    and space_pointer[0] <= coords[0] + coords[2]
                    and space_pointer[1] <= coords[1]
                    and space_pointer[1] >= coords[1] - coords[3]):
                    return win
            except Xlib.error.BadWindow as e:
                debug.DEBUG("get_active_window", "%s: %s" % (win.__window__(), e))

    return None

def get_event_window(display, event):
    if event == "ButtonPress":
        return get_active_window(display)
    else:
        return display.root.get("_NET_ACTIVE_WINDOW", None)
        #focus = display.get_input_focus().focus
        #if focus == Xlib.X.PointerRoot:
        #    return None
        return focus

def get_windows(display, view, margin=0.01, layers=("IG_LAYER_DESKTOP", "IG_LAYER_ISLAND")):
    visible = []
    overlap = []
    invisible = []
    for child in display.root.query_tree().children:
        if child.get_attributes().map_state != Xlib.X.IsViewable:
            continue

        child = child.find_client_window()
        if not child: continue
        if child.get("IG_LAYER", "IG_LAYER_DESKTOP") not in layers:
            continue
        coords = child["IG_COORDS"]
        
        # Margins to not get stuck due to rounding errors of
        # windows that sit right on the edge...
        marginx = view[2] * margin
        marginy = view[3] * margin

        win = list(coords)
        win[1] -= win[3]
            
        visiblex = win[0] + marginx >= view[0] and win[0] + win[2] - marginx <= view[0] + view[2]
        visibley = win[1] + marginy >= view[1] and win[1] + win[3] - marginy <= view[1] + view[3]
        
        invisiblex = win[0] + marginx >= view[0] + view[2] or win[0] + win[2] - marginx <= view[0]
        invisibley = win[1] + marginx >= view[1] + view[3] or win[1] + win[3] - marginx <= view[1]

        if visiblex and visibley:
            visible.append((child, coords))
        elif invisiblex or invisibley:
            invisible.append((child, coords))
        else:
            overlap.append((child, coords))
    return visible, overlap, invisible

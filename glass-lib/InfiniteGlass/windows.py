import Xlib.X
from . import coords as coordsmod 

def get_active_window(display):
    pointer = display.root.query_pointer()

    try:
        views = {display.root[name + "_LAYER"]: (display.root[name + "_VIEW"],
                                                      display.root[name + "_SIZE"])
                 for name in display.root["IG_VIEWS"]}
    except KeyError:
        return None
    spacecoords = {layer: coordsmod.view_to_space(view, size, pointer.root_x, pointer.root_y)
                   for layer, (view, size) in views.items()}
    for child in display.root.query_tree().children:
        if child.get_attributes().map_state != Xlib.X.IsViewable: continue
        coords = child.get("IG_COORDS", None)
        if coords is None: continue
        layer = child.get("IG_LAYER", "IG_LAYER_DESKTOP")
        pointer = spacecoords[layer]
        if (pointer[0] >= coords[0]
            and pointer[0] <= coords[0] + coords[2]
            and pointer[1] <= coords[1]
            and pointer[1] >= coords[1] - coords[3]):
            return child
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

def get_windows(display, view, margin=0.01, layer="IG_LAYER_DESKTOP"):
    visible = []
    invisible = []
    for child in display.root.query_tree().children:
        if child.get_attributes().map_state != Xlib.X.IsViewable:
            continue

        child = child.find_client_window()
        if not child: continue
        coords = child["IG_COORDS"]
        if child.get("IG_LAYER", "IG_LAYER_DESKTOP") != layer:
            continue
        
        # Margins to not get stuck due to rounding errors of
        # windows that sit right on the edge...
        marginx = view[2] * margin
        marginy = view[3] * margin
        if (    coords[0] + marginx >= view[0]
            and coords[0] + coords[2] - marginx <= view[0] + view[2]
            and coords[1] - coords[3] + marginy >= view[1]
            and coords[1] - marginy <= view[1] + view[3]):
            visible.append((child, coords))
        else:
            invisible.append((child, coords))
    return visible, invisible

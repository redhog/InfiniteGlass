event_mask_map = {
    "MotionNotify": ("ButtonMotionMask", "Button1MotionMask", "Button2MotionMask", "Button3MotionMask", "Button4MotionMask", "Button5MotionMask", "PointerMotionMask"),
    "ButtonPress": "ButtonPressMask",
    "ButtonRelease": "ButtonReleaseMask",
    "ColormapNotify": "ColormapChangeMask",
    "EnterNotify": "EnterWindowMask",
    "LeaveNotify": "LeaveWindowMask",
    "Expose": "ExposureMask",
    "NoExpose": "ExposureMask",
    "FocusIn": "FocusChangeMask",
    "FocusOut": "FocusChangeMask",
    "KeymapNotify": "KeymapStateMask",
    "KeyPress": "KeyPressMask",
    "KeyRelease": "KeyReleaseMask",
    "PropertyNotify": "PropertyChangeMask",
    "CirculateNotify": ("StructureNotifyMask", "SubstructureNotifyMask"),
    "ConfigureNotify": ("StructureNotifyMask", "SubstructureNotifyMask"),
    "CreateNotify": "SubstructureNotifyMask",
    "DestroyNotify": ("StructureNotifyMask", "SubstructureNotifyMask"),
    "GravityNotify": ("StructureNotifyMask", "SubstructureNotifyMask"),
    "MapNotify": ("StructureNotifyMask", "SubstructureNotifyMask"),
    "ReparentNotify": ("StructureNotifyMask", "SubstructureNotifyMask"),
    "UnmapNotify": ("StructureNotifyMask", "SubstructureNotifyMask"),
    "ResizeRequest": "ResizeRedirectMask",
    "CirculateRequest": "SubstructureRedirectMask",
    "ConfigureRequest": "SubstructureRedirectMask",
    "MapRequest": "SubstructureRedirectMask",
    "VisibilityNotify": "VisibilityChangeMask"
}
event_mask_map_inv = {}
for key, value in event_mask_map.items():
    if not isinstance(value, tuple): value = (value,)
    for item in value:
        if item in event_mask_map_inv:
            event_mask_map_inv[item] = tuple(set(event_mask_map_inv[item]).union((key,)))
        event_mask_map_inv[item] = (key,)

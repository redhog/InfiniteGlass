import Xlib.X

def find_client_window(win):
    try:
        if "WM_STATE" in win or "WM_NAME" in win:
            return win

        tree = win.query_tree()
    except Xlib.error.BadWindow:
        return None
        
    for child in tree.children:
        try:
            if "WM_STATE" in child or "WM_NAME" in child:
                return child
        except Xlib.error.BadWindow:
            pass
        
    for child in tree.children:
        try:
            attrs = child.get_attributes()
            if attrs.win_class != Xlib.X.InputOutput or attrs.map_state != Xlib.X.IsViewable: continue
        except Xlib.error.BadWindow:
            pass
        else:
            client = find_client_window(child)
            if client is not None:
                return client

    return None

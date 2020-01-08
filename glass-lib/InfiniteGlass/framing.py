import Xlib.X

def find_client_window(win):
    if "WM_STATE" in win or "WM_NAME" in win:
        return win

    tree = win.query_tree()

    for child in tree.children:
        if "WM_STATE" in child or "WM_NAME" in child:
            return child

    for child in tree.children:
        attrs = child.get_attributes()
        if attrs.win_class != Xlib.X.InputOutput or attrs.map_state != Xlib.X.IsViewable: continue
        client = find_client_window(child)
        if client is not None:
            return client

    return None

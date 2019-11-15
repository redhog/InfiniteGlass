import Xlib.X

def find_client_window(win):
    try:
        win["WM_STATE"]
    except KeyError:
        pass
    else:
        return win

    tree = win.query_tree()

    for child in tree.children:
        try:
            child["WM_STATE"]
        except KeyError:
            pass
        else:
            return child

    for child in tree.children:
        attrs = child.get_attributes()
        if attrs.win_class != Xlib.X.InputOutput or attrs.map_state != Xlib.X.IsViewable: continue
        client = find_client_window(child)
        if client is not None:
            return client

    return None

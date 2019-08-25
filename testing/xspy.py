import InfiniteGlass
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
        
with InfiniteGlass.Display() as display:
    @display.root.on(mask="SubstructureNotifyMask")
    def MapNotify(win, event):
        client_win = find_client_window(event.window) or event.window

        print("%s.map()" % (client_win.__window__(),))
        for name, value in client_win.items():
            try:
                print("%s.%s=%s" % (client_win.__window__(), name, client_win[name]))
            except Exception as e:
                print("%s.%s=???" % (client_win.__window__(), name))
        
        def PropertyNotify(win, event):
            name = display.get_atom_name(event.atom)
            try:
                print("%s.%s=%s" % (client_win.__window__(), name, client_win[name]))
            except:
                print("%s.%s=???" % (client_win.__window__(), name))
            
            
            

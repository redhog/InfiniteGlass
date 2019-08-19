import InfiniteGlass, Xlib.X
import struct

SET = ("IG_SIZE", "IG_COORDS")
MATCH = ("WM_CLASS", "WM_NAME")

windows = {}
shadows = {}

class Shadow(object):
    def __init__(self, display, properties):
        self.display = display
        self.properties = properties
        self.window = None
        print("SHADOW CREATE", self)
        
    def apply(self, window):
        print("SHADOW APPLY", window.__window__(), self)
        for key in SET:
            if key in self.properties:
                window[key] = self.properties[key]
        
    def activate(self):
        print("SHADOW ACTIVATE", self)
        self.window = display.root.create_window(map=False)
        self.window["IG_GHOST"] = "IG_GHOST"
        self.window["DISPLAYSVG"] = "@fontawesome-free-5.9.0-desktop/svgs/solid/bed.svg"
        self.window["WM_PROTOCOLS"] = ["WM_DELETE_WINDOW"]
        self.apply(self.window)

        @self.window.on(mask="NoEventMask", client_type="WM_PROTOCOLS")
        def ClientMessage(win, event):
            if event.parse("ATOM")[0] == "WM_DELETE_WINDOW":
                print("SHADOW DELETE", self)
                self.window.destroy()
                self.window.display.real_display.eventhandlers.remove(self.WMDelete)
                shadows.pop(self.key(), None)
            else:
                print("%s: Unknown WM_PROTOCOLS message: %s" % (self, event))

        self.WMDelete = ClientMessage
                
        self.window.map()

    def deactivate(self):
        print("SHADOW DEACTIVATE", self)
        if self.window is not None:
            self.window.destroy()
            self.window.display.real_display.eventhandlers.remove(self.WMDelete)

    def key(self):
        return tuple(self.properties.get(name, None) for name in sorted(MATCH))
        
    def __str__(self):
        return "/".join(str(item) for item in self.key())
            
class Window(object):
    def __init__(self, window):
        self.window = window
        self.id = self.window.__window__()
        self.shadow = None
        self.properties = {}
        for name, value in self.window.items():
            self.properties[name] = value
        print("WINDOW CREATE", self)
        self.match_shadow()
            
        @window.on()
        def PropertyNotify(win, event):
            name = self.window.display.real_display.get_atom_name(event.atom)
            try:
                self.properties[name] = win[name]
            except:
                pass
            else:
                self.match_shadow()
            
        @window.on(mask="StructureNotifyMask")
        def DestroyNotify(win, event):
            print("WINDOW DESTROY", self, event.window.__window__())
            if not self.shadow:
                self.shadow = Shadow(self.window.display.real_display, self.properties)
                key = tuple(self.properties.get(name, None) for name in sorted(MATCH))
                shadows[key] = self.shadow
            self.shadow.properties.update(self.properties)
            self.shadow.activate()
            windows.pop(self.id, None)
            self.window.display.real_display.eventhandlers.remove(PropertyNotify)
            self.window.display.real_display.eventhandlers.remove(DestroyNotify)
            
    def match_shadow(self):
        if self.shadow: return
        key = tuple(self.properties.get(name, None) for name in sorted(MATCH))
        if key in shadows:
            self.shadow = shadows[key]
            self.shadow.apply(self.window)
            self.shadow.deactivate()

    def key(self):
        return tuple(self.properties.get(name, None) for name in sorted(MATCH))
        
    def __str__(self):
        res = str(self.window.__window__())
        res += ": " + "/".join(str(item) for item in self.key())
        if self.shadow is not None:
            res += " (has shadow)"
        return res

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
        client_win = find_client_window(event.window)
        if client_win is None: return
        try:
            client_win["IG_GHOST"]
        except Exception as e:
            if client_win.__window__() not in windows:
                windows[client_win.__window__()] = Window(client_win)
            
    for child in display.root.query_tree().children:
        client_win = find_client_window(child)
        if client_win is None: continue
        if client_win.__window__() not in windows:
            windows[client_win.__window__()] = Window(client_win)

    print("Ghosts handler started")

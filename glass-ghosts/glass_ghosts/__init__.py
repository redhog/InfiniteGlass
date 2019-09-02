import InfiniteGlass, Xlib.X
import struct
import array
import pkg_resources
import sqlite3
import os.path
import json
import array
import base64

def tuplify(value):
    if isinstance(value, list):
        return tuple(value)
    return value
\

class Shadow(object):
    def __init__(self, manager, properties):
        self.manager = manager
        self.properties = properties
        self.window = None
        self.current_key = None
        self.update_key()
        print("SHADOW CREATE", self)

    def key(self):
        return tuple(tuplify(self.properties.get(name, None)) for name in sorted(self.manager.MATCH))

    def update_key(self):
        key = self.key()
        if key == self.current_key:
            return
        if self.current_key is not None:
            del self.manager.shadows[self.current_key]
        print("UPDATE KEY from %s to %s" % (self.current_key, key))
        self.current_key = key
        self.manager.shadows[self.current_key] = self

        if not self.manager.restoring_shadows:
            cur = self.manager.dbconn.cursor()
            dbkey = str(self)
            for name, value in self.properties.items():
                cur.execute("""
                  insert or replace into shadows (key, name, value) VALUES (?, ?, ?)
                """, (dbkey, name, json.dumps(value, default=self.manager.tojson)))
            self.manager.dbconn.commit()
            
    def apply(self, window):
        print("SHADOW APPLY", window.__window__(), self)
        for key in self.manager.SET:
            if key in self.properties:
                window[key] = self.properties[key]
        
    def activate(self):
        print("SHADOW ACTIVATE", self)
        self.window = self.manager.display.root.create_window(map=False)
        self.window["IG_GHOST"] = "IG_GHOST"
        
        with pkg_resources.resource_stream("glass_ghosts", "ghost.svg") as f:
            ghost_image = f.read()
        for name, value in self.properties.items():
            key = ("{%s}" % name).encode("utf-8")
            if not isinstance(value, (array.array, list, tuple)):
                value = [value]
            if value and isinstance(value[0], bytes):
                value = b'/'.join(item for item in value)
            else:
                value = b'/'.join(str(item).encode("utf-8")
                                  for item in value)
            if key in ghost_image:
                ghost_image = ghost_image.replace(key, value)
        self.window["DISPLAYSVG"] = ghost_image
        self.window["WM_PROTOCOLS"] = ["WM_DELETE_WINDOW"]
        self.apply(self.window)

        @self.window.on(mask="NoEventMask", client_type="WM_PROTOCOLS")
        def ClientMessage(win, event):
            if event.parse("ATOM")[0] == "WM_DELETE_WINDOW":
                print("SHADOW DELETE", self)
                self.window.destroy()
                self.manager.display.eventhandlers.remove(self.WMDelete)
                self.manager.shadows.pop(self.key(), None)
            else:
                print("%s: Unknown WM_PROTOCOLS message: %s" % (self, event))

        @self.window.on()
        def PropertyNotify(win, event):
            name = self.manager.display.get_atom_name(event.atom)
            try:
                self.properties[name] = win[name]
            except:
                pass
            else:
                self.update_key()
                
        self.WMDelete = ClientMessage
        self.PropertyNotify = PropertyNotify
                
        self.window.map()

    def deactivate(self):
        print("SHADOW DEACTIVATE", self)
        if self.window is not None:
            self.window.destroy()
            self.manager.display.eventhandlers.remove(self.PropertyNotify)
            self.manager.display.eventhandlers.remove(self.WMDelete)
        
    def __str__(self):
        return "/".join(str(item) for item in self.key())
            
class Window(object):
    def __init__(self, manager, window):
        self.manager = manager
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
            name = self.manager.display.get_atom_name(event.atom)
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
                self.shadow = Shadow(self.manager, self.properties)
            else:
                self.shadow.properties.update(self.properties)
                self.shadow.update_key()
            self.shadow.activate()
            self.manager.windows.pop(self.id, None)
            self.manager.display.eventhandlers.remove(PropertyNotify)
            self.manager.display.eventhandlers.remove(DestroyNotify)
            
    def key(self):
        return tuple(tuplify(self.properties.get(name, None)) for name in sorted(self.manager.MATCH))

    def match_shadow(self):
        if self.shadow: return
        key = self.key()
        if key in self.manager.shadows:
            self.shadow = self.manager.shadows[key]
            self.shadow.apply(self.window)
            self.shadow.deactivate()
        
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

class GhostManager(object):
    def __init__(self, display, MATCH = ("WM_CLASS", "WM_NAME"), SET = ("IG_SIZE", "IG_COORDS")):
        self.display = display

        self.MATCH = MATCH
        self.SET = SET

        self.windows = {}
        self.shadows = {}
        
        self.dbdirpath = os.path.expanduser("~/.config/glass")
        if not os.path.exists(self.dbdirpath):
            os.makedirs(self.dbdirpath)
        self.dbpath = os.path.join(self.dbdirpath, "ghosts.sqlite3")
        dbexists = os.path.exists(self.dbpath)
        self.dbconn = sqlite3.connect(self.dbpath)
        if not dbexists:
            self.dbconn.execute("create table shadows (key text, name text, value text, primary key (key, name))")

        self.restore_shadows()
            
        @display.root.on(mask="SubstructureNotifyMask")
        def MapNotify(win, event):
            client_win = find_client_window(event.window)
            if client_win is None: return
            try:
                client_win["IG_GHOST"]
            except Exception as e:
                if client_win.__window__() not in self.windows:
                    self.windows[client_win.__window__()] = Window(self, client_win)

        for child in display.root.query_tree().children:
            client_win = find_client_window(child)
            if client_win is None: continue
            if client_win.__window__() not in self.windows:
                self.windows[client_win.__window__()] = Window(self, client_win)

        print("Ghosts handler started")
            
    def restore_shadows(self):
        self.restoring_shadows = True
        cur = self.dbconn.cursor()
        cur.execute("select * from shadows order by key")
        properties = {}
        currentkey = None
        for key, name, value in cur:
            if key != currentkey:
                if currentkey:
                    Shadow(self, properties).activate()
                properties = {}
                currentkey = key
            properties[name] = json.loads(value, object_hook=self.fromjson)
        if currentkey:
            Shadow(self, properties).activate()
        self.restoring_shadows = False


    def tojson(self, obj):
        if isinstance(obj, array.array):
            return list(obj)
        if isinstance(obj, bytes):
            return {"__jsonclass__": ["base64", base64.b64encode(obj).decode("ascii")]}
        if type(obj).__name__ == "Window":
            return {"__jsonclass__": ["Window", obj.__window__()]}
        return obj

    def fromjson(self, obj):
        if "__jsonclass__" in obj:
            cls = obj.pop("__jsonclass__")
            if cls[0] == "base64":
                return base64.b64decode(cls[1])
            if cls[0] == "Window":
                return self.display.create_resource_object("window", cls[1])
        return obj

        
with InfiniteGlass.Display() as display:
    GhostManager(display)

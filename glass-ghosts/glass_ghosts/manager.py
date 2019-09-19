import InfiniteGlass, Xlib.X
import struct
import array
import pkg_resources
import sqlite3
import os.path
import json
import array
import base64
import glass_ghosts.shadow
import glass_ghosts.window
import sys

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

        self.changes = False
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

        def map_window(win):
            if win.get_attributes().override_redirect:
                return
            
            client_win = find_client_window(win)
            if client_win is None: return
            try:
                client_win["IG_GHOST"]
            except Exception as e:
                if client_win.__window__() not in self.windows:
                    self.windows[client_win.__window__()] = glass_ghosts.window.Window(self, client_win)

        display.mainloop.add_interval(0.5)(self.save_shadows)
                    
        @display.root.on(mask="SubstructureNotifyMask")
        def MapNotify(win, event):
            map_window(event.window)
            
        for child in display.root.query_tree().children:
            map_window(child)

        sys.stderr.write("Ghosts handler started\n"); sys.stderr.flush()

    def save_shadows(self, current_time, idx):
        if self.changes:
            print("Committing...")
            sys.stdout.flush()
            self.dbconn.commit()
            self.changes = False
    
    def restore_shadows(self):
        self.restoring_shadows = True
        cur = self.dbconn.cursor()
        cur.execute("select * from shadows order by key")
        properties = {}
        currentkey = None
        for key, name, value in cur:
            if key != currentkey:
                if currentkey:
                    glass_ghosts.shadow.Shadow(self, properties).activate()
                properties = {}
                currentkey = key
            properties[name] = json.loads(value, object_hook=self.fromjson)
        if currentkey:
            glass_ghosts.shadow.Shadow(self, properties).activate()
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

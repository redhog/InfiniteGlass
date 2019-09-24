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
import glass_ghosts.rootwindow
import glass_ghosts.session
import sys

class GhostManager(object):
    def __init__(self, display, MATCH = ("WM_CLASS", "WM_NAME"), SET = ("IG_SIZE", "IG_COORDS")):
        self.display = display

        self.MATCH = MATCH
        self.SET = SET

        self.changes = False
        self.windows = {}
        self.shadows = {}
        self.clients = {}
        
        self.dbdirpath = os.path.expanduser("~/.config/glass")
        if not os.path.exists(self.dbdirpath):
            os.makedirs(self.dbdirpath)
        self.dbpath = os.path.join(self.dbdirpath, "ghosts.sqlite3")
        dbexists = os.path.exists(self.dbpath)
        self.dbconn = sqlite3.connect(self.dbpath)
        if not dbexists:
            self.dbconn.execute("create table shadows (key text, name text, value text, primary key (key, name))")
            self.dbconn.execute("create table clients (key text, name text, value text, primary key (key, name))")

        self.session = glass_ghosts.session.Server(self, display)
        self.rootwindow = glass_ghosts.rootwindow.RootWindow(self, display)

        self.restore_shadows()
        self.restore_clients()
        
        display.mainloop.add_interval(0.5)(self.save_shadows)

        sys.stderr.write("Ghosts handler started\n"); sys.stderr.flush()
        sys.stderr.flush()

    def save_shadows(self, current_time, idx):
        if self.changes:
            #sys.stderr.write("Committing...\n")
            #sys.stderr.flush()
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

    def restore_clients(self):
        self.restoring_clients = True
        cur = self.dbconn.cursor()
        cur.execute("select * from clients order by key")
        properties = {}
        currentkey = None
        for key, name, value in cur:
            if key != currentkey:
                if currentkey:
                    client = glass_ghosts.client.Client(self, currentkey, properties)
                    self.clients[client.client_id] = client
                properties = {}
                currentkey = key
            properties[name] = json.loads(value, object_hook=self.fromjson)
        if currentkey:
            client = glass_ghosts.client.Client(self, currentkey, properties)
            self.clients[client.client_id] = client
        self.restoring_clients = False

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

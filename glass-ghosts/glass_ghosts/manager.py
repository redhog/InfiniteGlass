import InfiniteGlass
import array
import sqlite3
import os.path
import yaml
import json
import base64
import glass_ghosts.shadow
import glass_ghosts.window
import glass_ghosts.rootwindow
import glass_ghosts.components
import glass_ghosts.session
import pkg_resources

class GhostManager(object):
    def __init__(self, display):

        configpath = os.environ.get("GLASS_GHOSTS_CONFIG", "~/.config/glass/ghosts.json")
        if configpath:
            configpath = os.path.expanduser(configpath)

            configdirpath = os.path.dirname(configpath)
            if not os.path.exists(configdirpath):
                os.makedirs(configdirpath)

            if not os.path.exists(configpath):
                with pkg_resources.resource_stream("glass_ghosts", "config.json") as inf:
                    with open(configpath, "wb") as outf:
                        outf.write(inf.read())

            with open(configpath) as f:
                self.config = yaml.load(f, Loader=yaml.SafeLoader)
        else:
            with pkg_resources.resource_stream("glass_ghosts", "config.json") as f:
                self.config = yaml.load(f, Loader=yaml.SafeLoader)
       
        self.display = display

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
        self.components = glass_ghosts.components.Components(self, display)

        self.restore_config_shadows()
        self.restore_shadows()
        self.restore_clients()

        display.mainloop.add_interval(0.5)(self.save_shadows)
                
        InfiniteGlass.DEBUG("init", "Ghosts handler started\n")

    def save_shadows(self, current_time, idx):
        if self.changes:
            InfiniteGlass.DEBUG("conmmit", "Committing...\n")
            self.dbconn.commit()
            self.changes = False

    def restore_config_shadows(self):
        self.restoring_shadows = True
        shadows = json.loads(json.dumps(self.config.get("shadows", {})), object_hook=self.fromjson)
        for key, properties in shadows.items():
            glass_ghosts.shadow.Shadow(self, properties, from_config=True).activate()
        self.restoring_shadows = False
        
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
            return {"__jsonclass__": ["array", obj.typecode, list(obj)]}
        elif isinstance(obj, bytes):
            try:
                return {"__jsonclass__": ["string", obj.decode("utf-8")]}
            except:
                return {"__jsonclass__": ["base64", base64.b64encode(obj).decode("ascii")]}
        elif type(obj).__name__ == "Window":
            return {"__jsonclass__": ["Window", obj.__window__()]}
        return obj

    def fromjson(self, obj):
        if "__jsonclass__" in obj:
            cls = obj.pop("__jsonclass__")
            if cls[0] == "array":
                return array.array(cls[1], cls[2])
            elif cls[0] == "string":
                return cls[1].encode("utf-8")
            elif cls[0] == "base64":
                return base64.b64decode(cls[1])
            elif cls[0] == "Window":
                return self.display.create_resource_object("window", cls[1])
        return obj

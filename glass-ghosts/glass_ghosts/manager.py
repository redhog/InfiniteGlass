import InfiniteGlass
import array
import sqlite3
import os.path
import yaml
import json
import base64
import glass_ghosts.ghost
import glass_ghosts.window
import glass_ghosts.rootwindow
import glass_ghosts.components
import glass_ghosts.session
import pkg_resources
import sys

class GhostManager(object):
    def __init__(self, display, **kw):

        self.display = display

        configpath = os.path.expanduser(os.environ.get("GLASS_GHOSTS_CONFIG", "~/.config/glass/ghosts.yml"))
        with open(configpath) as f:
            self.config = json.loads(json.dumps(yaml.load(f, Loader=yaml.SafeLoader)), object_hook=InfiniteGlass.fromjson(self.display))

        self.changes = False
        self.windows = {}
        self.ghosts = {}
        self.clients = {}

        self.dbdirpath = os.path.expanduser("~/.config/glass")
        if not os.path.exists(self.dbdirpath):
            os.makedirs(self.dbdirpath)
        self.dbpath = os.path.join(self.dbdirpath, "ghosts.sqlite3")
        dbexists = os.path.exists(self.dbpath)
        self.dbconn = sqlite3.connect(self.dbpath)
        if not dbexists:
            self.dbconn.execute("create table ghosts (key text, name text, value text, primary key (key, name))")
            self.dbconn.execute("create table clients (key text, name text, value text, primary key (key, name))")

        # Order is important here...
        self.restore_clients()
        self.restore_config_ghosts()
        self.restore_ghosts()

        self.session = glass_ghosts.session.Server(self, display, **kw)
        self.rootwindow = glass_ghosts.rootwindow.RootWindow(self, display, **kw)
        self.components = glass_ghosts.components.Components(self, display, **kw)

        display.mainloop.add_interval(0.5)(self.save_ghosts)
                
        InfiniteGlass.DEBUG("init", "Ghosts handler started\n")

    def shutdown(self):
        @self.display.mainloop.add_interval(0.1)
        def attempt_shutdown(timestamp, idx):
            waiting = 0
            for client_id, client in self.clients.items():
                for fd, conn in list(client.connections.items()):
                    conn.sleep()
                    waiting += 1
            if not waiting:
                InfiniteGlass.DEBUG("conmmit", "Committing...\n")
                self.dbconn.commit()
                self.display.exit()
        
    def save_ghosts(self, current_time, idx):
        if self.changes:
            InfiniteGlass.DEBUG("conmmit", "Committing...\n")
            self.dbconn.commit()
            self.changes = False

    def restore_config_ghosts(self):
        self.restoring_ghosts = True
        ghosts = self.config.get("ghosts", {})
        for properties in ghosts:
            glass_ghosts.ghost.Shadow(self, properties, from_config=True).activate()
        self.restoring_ghosts = False
        
    def restore_ghosts(self):
        self.restoring_ghosts = True
        cur = self.dbconn.cursor()
        cur.execute("select * from ghosts order by key")
        properties = {}
        currentkey = None
        for key, name, value in cur:
            if key != currentkey:
                if currentkey:
                    glass_ghosts.ghost.Shadow(self, properties).activate()
                properties = {}
                currentkey = key
            properties[name] = json.loads(value, object_hook=InfiniteGlass.fromjson(self.display))
        if currentkey:
            glass_ghosts.ghost.Shadow(self, properties).activate()
        self.restoring_ghosts = False

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
            properties[name] = json.loads(value, object_hook=InfiniteGlass.fromjson(self.display))
        if currentkey:
            client = glass_ghosts.client.Client(self, currentkey, properties)
            self.clients[client.client_id] = client
        self.restoring_clients = False

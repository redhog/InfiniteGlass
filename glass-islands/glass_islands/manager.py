import InfiniteGlass
import json
import yaml
import uuid
import os
import sqlite3
import sys
import pkg_resources
import array
from . import island

class IslandManager(object):
    def __init__(self, display):

        self.display = display

        configpath = os.path.expanduser(os.environ.get("GLASS_ISLANDS_CONFIG", "~/.config/glass/islands.yml"))
        with open(configpath) as f:
            self.config = json.loads(json.dumps(yaml.load(f, Loader=yaml.SafeLoader)), object_hook=InfiniteGlass.fromjson(self.display))

        self.changes = False
        self.islands = {}

        self.dbdirpath = os.path.expanduser("~/.config/glass")
        if not os.path.exists(self.dbdirpath):
            os.makedirs(self.dbdirpath)
        self.dbpath = os.path.join(self.dbdirpath, "islands.sqlite3")
        dbexists = os.path.exists(self.dbpath)
        self.dbconn = sqlite3.connect(self.dbpath)
        if not dbexists:
            self.dbconn.execute("create table islands (key text, name text, value text, primary key (key, name))")

        self.restore_islands()

        display.mainloop.add_interval(0.5)(self.save_islands)

        @display.root.on(mask="StructureNotifyMask", client_type="IG_ISLAND_CREATE")
        def ClientMessage(win, event):
            print("message", "RECEIVED CREATE"); sys.stderr.flush()
            self.create()
        
        InfiniteGlass.DEBUG("init", "Islands handler started\n")

    def shutdown(self):
        self.dbconn.commit()
        sys.exit(0)
        
    def save_islands(self, current_time, idx):
        if self.changes:
            InfiniteGlass.DEBUG("conmmit", "Committing...\n")
            self.dbconn.commit()
            self.changes = False

    def create(self):
        island.Island(self, {})
            
    def restore_islands(self):
        self.restoring_islands = True
        cur = self.dbconn.cursor()
        cur.execute("select * from islands order by key")
        properties = {}
        currentkey = None
        for key, name, value in cur:
            if key != currentkey:
                if currentkey:
                    island.Island(self, properties, key=currentkey)
                properties = {}
                currentkey = key
            properties[name] = json.loads(value, object_hook=InfiniteGlass.fromjson(self.display))
        if currentkey:
            island.Island(self, properties, key=currentkey)
        self.restoring_islands = False

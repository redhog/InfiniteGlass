import InfiniteGlass
import json
import yaml
import uuid
import os
import sqlite3
import sys
import pkg_resources
import array

class NoValue: pass

class Island(object):
    def __init__(self, manager, properties=None, key=None):
        self.manager = manager
        if not properties: properties = {}
        if "IG_COORDS" not in properties:
            coords = self.manager.display.root["IG_VIEW_DESKTOP_VIEW"]
            properties["IG_COORDS"] = [coords[0], coords[1]+coords[3], coords[2], coords[3]]
        self._properties = {}
        self.properties = properties
        self.key = key or str(uuid.uuid4())
        self.window = None
        self.manager.islands[self.key] = self
        self.activate()
        InfiniteGlass.DEBUG("island", "ISLAND CREATE %s\n" % (self,)); sys.stderr.flush()
        
    def activate(self):
        self.window = self.manager.display.root.create_window(map=False, **self.properties.get("__attributes__", {}))

        self.window["IG_WINDOW_TYPE"] = "IG_ISLAND"
        self.window["IG_ISLAND_ID"] = self.key
        self.window["_NET_WM_WINDOW_TYPE"] = "_NET_WM_WINDOW_TYPE_DESKTOP"
        self.window["IG_LAYER"] = "IG_LAYER_ISLAND"
        self.window["WM_PROTOCOLS"] = ["WM_DELETE_WINDOW"]
        for key, value in self.properties.items():
            InfiniteGlass.DEBUG("island.properties", "%s=%s\n" % (key, str(value)[:100])); sys.stderr.flush()
            try:
                self.window[key] = value
            except:
                pass
                
        self.load_image()

        @self.window.on(mask="StructureNotifyMask")
        def DestroyNotify(win, event):
            InfiniteGlass.DEBUG("island", "ISLAND DELETE %s\n" % (self,)); sys.stderr.flush()
            self.destroy()
        self.DestroyNotify = DestroyNotify

        @self.window.on(mask="StructureNotifyMask", client_type="IG_CLOSE")
        def ClientMessage(win, event):
            win.destroy()
        self.CloseMessage = ClientMessage

        @self.window.on(mask="NoEventMask", client_type="WM_PROTOCOLS")
        def ClientMessage(win, event):
            if event.parse("ATOM")[0] == "WM_DELETE_WINDOW":
                win.destroy()
            else:
                InfiniteGlass.DEBUG("island", "%s: Unknown WM_PROTOCOLS message: %s\n" % (self, event)); sys.stderr.flush()
        self.WMDelete = ClientMessage

        @self.window.on()
        def PropertyNotify(win, event):
            name = self.manager.display.get_atom_name(event.atom)
            try:
                self.properties[name] = win[name]
                InfiniteGlass.DEBUG("island.property", "%s=%s\n" % (name, self.properties[name])); sys.stderr.flush()
            except:
                pass
            else:
                self.save_changes()
            InfiniteGlass.DEBUG("setprop", "%s=%s" % (name, self.properties.get(name)))
        self.PropertyNotify = PropertyNotify
        
        @self.window.on(mask="StructureNotifyMask", client_type="IG_RESTART")
        def ClientMessage(win, event):
            InfiniteGlass.DEBUG("island", "ISLAND RESTART %s\n" % (self,)); sys.stderr.flush()
            if "SM_CLIENT_ID" not in self.properties: return
            self.manager.clients[self.properties["SM_CLIENT_ID"]].restart()
        self.RestartMessage = ClientMessage

        @self.window.on()
        def Expose(win, event):
            self.redraw()
        self.Expose = Expose        

        self.window.map()
        self.redraw()

    def format_pair(self, name, value, sep=b"/"):
        pattern = ("{%s}" % name).encode("utf-8")
        if not isinstance(value, (array.array, list, tuple)):
            value = [value]
        if value and isinstance(value[0], bytes):
            value = sep.join(item for item in value)
        else:
            value = sep.join(str(item).encode("utf-8")
                              for item in value)
        return pattern, value

    def load_image(self):
        with pkg_resources.resource_stream("glass_islands", "island.svg") as f:
            island_image = f.read()
        for name, value in self.properties.items():
            pattern, value = self.format_pair(name, value)
            if pattern in island_image:
                island_image = island_image.replace(pattern, value)
        pattern, value = self.format_pair("key", self)
        if pattern in island_image:
            island_image = island_image.replace(pattern, value)
        self.window["IG_CONTENT"] = ("IG_SVG", island_image)
        
    def save_changes(self):
        if not self.manager.restoring_islands:
            cur = self.manager.dbconn.cursor()
            for name, value in self.properties.items():
                if self._properties.get(name, NoValue) != value:
                    cur.execute("""
                      insert or replace into islands (key, name, value) VALUES (?, ?, ?)
                    """, (self.key, name, json.dumps(value, default=InfiniteGlass.tojson(self.manager.display))))
                    self.manager.changes = True
            for name, value in self._properties.items():
                if name not in self.properties:
                    cur.execute("""
                      delete from islands where key=? and name=?
                    """, (self.key, name))
                    self.manager.changes = True

    def redraw(self):
        gcbg = self.manager.display.root.create_gc(foreground=self.manager.display.screen(0).white_pixel,
                                                   background=self.manager.display.screen(0).black_pixel)
        gcfg = self.manager.display.root.create_gc(foreground=self.manager.display.screen(0).black_pixel,
                                                   background=self.manager.display.screen(0).white_pixel)
        geom = self.window.get_geometry()
        self.window.fill_rectangle(gcbg, 0, 0, geom.width, geom.height)
        self.window.draw_text(gcfg, 10, 10, str(self.properties.get("WM_NAME", "")))
        self.window.draw_text(gcfg, 10, 30, str(self.properties.get("WM_CLASS", "")))
        self.manager.display.flush()

    def destroy(self):
        InfiniteGlass.DEBUG("island", "ISLAND DESTROY %s\n" % (self,)); sys.stderr.flush()
        try:

            if self.window is not None:
                self.window.destroy()
                self.manager.display.eventhandlers.remove(self.RestartMessage)
                self.manager.display.eventhandlers.remove(self.DestroyNotify)
                self.manager.display.eventhandlers.remove(self.PropertyNotify)
                self.manager.display.eventhandlers.remove(self.WMDelete)
                self.manager.display.eventhandlers.remove(self.CloseMessage)
                self.manager.display.eventhandlers.remove(self.Expose)
                self.window = None
        except Exception as e:
            print("Error closing window", e)

        self.manager.islands.pop(self.key, None)
        
        cur = self.manager.dbconn.cursor()
        cur.execute("""
            delete from islands where key = ?
        """, (self.key,))
        self.manager.dbconn.commit()
        print("XXXXXXXXXXXXXXXXXXXXXXXXX")

    def __str__(self):
        res = self.key
        if "WM_NAME" in self.properties:
            res = "%s: %s" % (res, self.properties["WM_NAME"])
        return res

class IslandManager(object):
    def __init__(self, display):

        self.display = display

        configpath = os.path.expanduser(os.environ.get("GLASS_ISLANDS_CONFIG", "~/.config/glass/islands.json"))
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
        Island(self, {})
            
    def restore_islands(self):
        self.restoring_islands = True
        cur = self.dbconn.cursor()
        cur.execute("select * from islands order by key")
        properties = {}
        currentkey = None
        for key, name, value in cur:
            if key != currentkey:
                if currentkey:
                    Island(self, properties, key=currentkey)
                properties = {}
                currentkey = key
            properties[name] = json.loads(value, object_hook=InfiniteGlass.fromjson(self.display))
        if currentkey:
            Island(self, properties, key=currentkey)
        self.restoring_islands = False

def main():
    with InfiniteGlass.Display() as display:
        manager = IslandManager(display)
            

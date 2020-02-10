import InfiniteGlass
import array
import pkg_resources
import json
import glass_ghosts.helpers
import sys

class NoValue: pass

class Shadow(object):
    def __init__(self, manager, properties, from_config=False):
        self.manager = manager
        self._properties = {}
        self.properties = properties
        self.from_config = from_config
        self.window = None
        self.current_key = None
        self.update_key()
        InfiniteGlass.DEBUG("ghost", "SHADOW CREATE %s\n" % (self,)); sys.stderr.flush()

    def key(self):
        return glass_ghosts.helpers.ghost_key(self.properties, self.manager.config["match"])

    def update_key(self):
        key = self.key()

        if key != self.current_key and key in self.manager.ghosts:
            InfiniteGlass.DEBUG("ghost", "DUPLICATE SHADOW %s\n" % (self,))
            self.destroy()

        if not self.manager.restoring_ghosts and not self.from_config:

            cur = self.manager.dbconn.cursor()
            for name, value in self.properties.items():
                if self._properties.get(name, NoValue) != value:
                    cur.execute("""
                      insert or replace into ghosts (key, name, value) VALUES (?, ?, ?)
                    """, (key, name, json.dumps(value, default=InfiniteGlass.tojson(self.manager.display))))
                    self.manager.changes = True
            for name, value in self._properties.items():
                if name not in self.properties:
                    cur.execute("""
                      delete from ghosts where key=? and name=?
                    """, (key, name))
                    self.manager.changes = True
            if key != self.current_key and self.current_key is not None:
                try:
                    cur.execute("""
                      update ghosts set key=? where key=?
                        """, (key, self.current_key))
                except Exception as e:
                    InfiniteGlass.DEBUG("ghost.database", "Error updating key in db: %s\nkey=%s =>  key=%s\n" % (
                        e, self.current_key, key))
                    self.destroy()
                self.manager.changes = True

        if key == self.current_key:
            return

        client = self.manager.clients.get(self.properties.get("SM_CLIENT_ID"))
        
        if self.current_key is not None:
            del self.manager.ghosts[self.current_key]
            if client:
                del client.ghosts[self.current_key]
            InfiniteGlass.DEBUG("ghost", "UPDATE KEY from %s to %s\n" % (self.current_key, key)); sys.stderr.flush()

        self.current_key = key
        self.manager.ghosts[self.current_key] = self

        if client:
            client.ghosts[self.current_key] = self

    def apply(self, window, type="set"):
        InfiniteGlass.DEBUG("ghost", "SHADOW APPLY window_id=%s %s\n" % (window.__window__(), self)); sys.stderr.flush()
        for key in self.manager.config[type]:
            if key in self.properties:
                InfiniteGlass.DEBUG("ghost.properties", "%s=%s\n" % (key, str(self.properties[key])[:100])); sys.stderr.flush()
                window[key] = self.properties[key]

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
    
    def activate(self):
        InfiniteGlass.DEBUG("ghost", "SHADOW ACTIVATE %s\n" % (self,)); sys.stderr.flush()
        for name, value in self.properties.items():
            InfiniteGlass.DEBUG("ghost.properties", "%s=%s\n" % (name, str(value)[:100])); sys.stderr.flush()
        
        self.window = self.manager.display.root.create_window(map=False, **self.properties.get("__attributes__", {}))
        self.window["IG_GHOST"] = "IG_GHOST"

        with pkg_resources.resource_stream("glass_ghosts", "ghost.svg") as f:
            ghost_image = f.read()
        
        for name, value in self.properties.items():
            pattern, value = self.format_pair(name, value)
            if pattern in ghost_image:
                ghost_image = ghost_image.replace(pattern, value)

        pattern, value = self.format_pair("key", self.key())
        if pattern in ghost_image:
            ghost_image = ghost_image.replace(pattern, value)

        if "SM_CLIENT_ID" in self.properties:
            if self.properties["SM_CLIENT_ID"] in self.manager.clients:
                for name, value in self.manager.clients[self.properties["SM_CLIENT_ID"]].properties.items():
                    pattern, value = self.format_pair(name, value[1], b" ")
                    if pattern in ghost_image:
                        ghost_image = ghost_image.replace(pattern, value)
        else:
            pattern, value = self.format_pair("SM_CLIENT_ID", "No state saved")
            if pattern in ghost_image:
                ghost_image = ghost_image.replace(pattern, value)
                    
        self.window["IG_CONTENT"] = ("IG_SVG", ghost_image)
        self.window["WM_PROTOCOLS"] = ["WM_DELETE_WINDOW"]
        self.apply(self.window, type="ghost_set")

        @self.window.on(mask="StructureNotifyMask")
        def DestroyNotify(win, event):
            InfiniteGlass.DEBUG("ghost", "SHADOW DELETE %s\n" % (self,)); sys.stderr.flush()
            self.destroy()

        @self.window.on(mask="StructureNotifyMask", client_type="IG_CLOSE")
        def ClientMessage(win, event):
            self.window.destroy()
        self.CloseMessage = ClientMessage

        @self.window.on(mask="NoEventMask", client_type="WM_PROTOCOLS")
        def ClientMessage(win, event):
            if event.parse("ATOM")[0] == "WM_DELETE_WINDOW":
                self.window.destroy()
            else:
                InfiniteGlass.DEBUG("ghost", "%s: Unknown WM_PROTOCOLS message: %s\n" % (self, event)); sys.stderr.flush()
        self.WMDelete = ClientMessage

        @self.window.on()
        def PropertyNotify(win, event):
            name = self.manager.display.get_atom_name(event.atom)
            if name not in self.manager.config["ghost_update"]: return
            try:
                self.properties.update(glass_ghosts.helpers.expand_property(win, name))
                InfiniteGlass.DEBUG("ghost.property", "%s=%s\n" % (name, self.properties[name])); sys.stderr.flush()
            except:
                pass
            else:
                self.update_key()
            InfiniteGlass.DEBUG("setprop", "%s=%s" % (name, self.properties.get(name)))
            
        @self.window.on()
        def ButtonPress(win, event):
            if "SM_CLIENT_ID" not in self.properties: return
            self.manager.clients[self.properties["SM_CLIENT_ID"]].restart()

        @self.window.on()
        def Expose(win, event):
            self.redraw()

        self.Expose = Expose
        self.DestroyNotify = DestroyNotify
        self.PropertyNotify = PropertyNotify
        self.ButtonPress = ButtonPress

        self.window.map()
        self.redraw()

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

    def deactivate(self):
        InfiniteGlass.DEBUG("ghost", "SHADOW DEACTIVATE %s\n" % (self,)); sys.stderr.flush()
        if self.window is not None:
            self.window.destroy()
            self.manager.display.eventhandlers.remove(self.ButtonPress)
            self.manager.display.eventhandlers.remove(self.DestroyNotify)
            self.manager.display.eventhandlers.remove(self.PropertyNotify)
            self.manager.display.eventhandlers.remove(self.WMDelete)
            self.manager.display.eventhandlers.remove(self.CloseMessage)
            self.manager.display.eventhandlers.remove(self.Expose)
            self.window = None

    def destroy(self):
        self.deactivate()

        self.manager.ghosts.pop(self.key(), None)
        client = self.manager.clients.get(self.properties.get("SM_CLIENT_ID"))
        if client:
            client.ghosts.pop(self.key(), None)
        
        if not self.from_config:
            cur = self.manager.dbconn.cursor()
            cur.execute("""
                delete from ghosts where key = ?
            """, (self.key(),))
            self.manager.dbconn.commit()

    def __str__(self):
        return self.key()

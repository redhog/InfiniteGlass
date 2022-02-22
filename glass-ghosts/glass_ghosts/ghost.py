import InfiniteGlass
import array
import pkg_resources
import json
import glass_ghosts.helpers
import sys
import os
import re

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
            try:
                del self.manager.ghosts[self.current_key]
            except:
                InfiniteGlass.DEBUG("ghost", "OLD KEY %s is missing\n" % (self.current_key,)); sys.stderr.flush()
            if client:
                try:
                    del client.ghosts[self.current_key]
                except:
                    InfiniteGlass.DEBUG("ghost", "OLD KEY %s is missing on client %s\n" % (self.current_key, client)); sys.stderr.flush()
            InfiniteGlass.DEBUG("ghost", "UPDATE KEY from %s to %s\n" % (self.current_key, key)); sys.stderr.flush()

        self.current_key = key
        self.manager.ghosts[self.current_key] = self

        if client:
            client.ghosts[self.current_key] = self

    def apply(self, window, type="set"):
        InfiniteGlass.DEBUG("ghost.apply", "SHADOW APPLY %s window_id=%s %s\n" % (type, window, self)); sys.stderr.flush()
        if self.properties.get("IG_GHOSTS_DISABLED", 0):
            window["IG_GHOSTS_DISABLED"] = 1
        else:
            for key in self.manager.config[type]:
                if key in self.properties:
                    if InfiniteGlass.DEBUG_ENABLED("ghost.apply.properties"):
                        itemtype, items, fmt = InfiniteGlass.parse_value(self.manager.display, self.properties[key])
                        InfiniteGlass.DEBUG("ghost.properties", "%s=%s\n" % (key, str(items)[:100])); sys.stderr.flush()
                    try:
                        window[key] = self.properties[key]
                    except Exception as e:
                        InfiniteGlass.ERROR("ghost.properties", "Unable to set property %s.%s=%s\n" % (window, key, self.properties[key]))
                    else:
                        InfiniteGlass.DEBUG("ghost.properties", "    => %s=%s\n" % (key, window[key]))
        self.manager.display.flush()
        
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
        if self.properties.get("IG_GHOSTS_DISABLED", 0):
            return
            
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
            if "WM_COMMAND" in self.properties:
                pattern, value = self.format_pair("RestartCommand", self.properties["WM_COMMAND"])
                if pattern in ghost_image:
                    ghost_image = ghost_image.replace(pattern, value)
                
        self.window["IG_CONTENT"] = ("IG_SVG", ghost_image)
        self.window["WM_PROTOCOLS"] = ["WM_DELETE_WINDOW"]
        self.apply(self.window, type="ghost_set")

        @self.window.on(mask="StructureNotifyMask")
        def DestroyNotify(win, event):
            InfiniteGlass.DEBUG("ghost", "GHOST DESTROY %s\n" % (self,)); sys.stderr.flush()
            self.destroy()
        self.DestroyNotify = DestroyNotify

        @self.window.on(mask="StructureNotifyMask", client_type="IG_CLOSE")
        def ClientMessage(win, event):
            InfiniteGlass.DEBUG("ghost", "GHOST CLOSE %s\n" % (self,)); sys.stderr.flush()
            win.destroy()
        self.CloseMessage = ClientMessage

        @self.window.on(mask="StructureNotifyMask", client_type="IG_DELETE")
        def ClientMessage(win, event):
            InfiniteGlass.DEBUG("ghost", "GHOST DELETE %s\n" % (self,)); sys.stderr.flush()
            win.destroy()
        self.DeleteMessage = ClientMessage

        @self.window.on(mask="NoEventMask", client_type="WM_PROTOCOLS")
        def ClientMessage(win, event):
            InfiniteGlass.DEBUG("ghost", "GHOST WM_DELETE_WINDOW %s\n" % (self,)); sys.stderr.flush()
            if event.parse("ATOM")[0] == "WM_DELETE_WINDOW":
                win.destroy()
            else:
                InfiniteGlass.DEBUG("ghost", "%s: Unknown WM_PROTOCOLS message: %s\n" % (self, event)); sys.stderr.flush()
        self.WMDelete = ClientMessage

        @self.window.on()
        def PropertyNotify(win, event):
            name = self.manager.display.get_atom_name(event.atom)
            if name not in self.manager.config["ghost_update"]: return
            try:
                self.properties.update(glass_ghosts.helpers.expand_property(win, name))
                InfiniteGlass.DEBUG("ghost.update.property", "%s.%s=%s from %s\n" % (self, name, self.properties[name], win)); sys.stderr.flush()
            except:
                pass
            else:
                self.update_key()
        self.PropertyNotify = PropertyNotify
            
        @self.window.on()
        def ButtonPress(win, event):
            self.restart()
        self.ButtonPress = ButtonPress

        @self.window.on(mask="StructureNotifyMask", client_type="IG_RESTART")
        def ClientMessage(win, event):
            self.restart()
        self.RestartMessage = ClientMessage

        @self.window.on()
        def Expose(win, event):
            self.redraw()
        self.Expose = Expose        

        self.window.map()
        self.redraw()

    def restart(self):
        InfiniteGlass.DEBUG("ghost", "GHOST RESTART %s\n" % (self,)); sys.stderr.flush()
        if "SM_CLIENT_ID" in self.properties:
            self.manager.clients[self.properties["SM_CLIENT_ID"]].restart()
        elif "WM_COMMAND" in self.properties:
           if os.fork() == 0:
               cmd = self.properties["WM_COMMAND"]
               if not isinstance(cmd, list): cmd = [cmd]
               cmd = [name.decode("utf-8") for name in cmd]
               os.execlp(cmd[0], *cmd)
            
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
            self.manager.display.eventhandlers.remove(self.RestartMessage)
            self.manager.display.eventhandlers.remove(self.ButtonPress)
            self.manager.display.eventhandlers.remove(self.DestroyNotify)
            self.manager.display.eventhandlers.remove(self.PropertyNotify)
            self.manager.display.eventhandlers.remove(self.WMDelete)
            self.manager.display.eventhandlers.remove(self.DeleteMessage)
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

    @property
    def is_active(self):
        return self.window is not None
            
    def __str__(self):
        return "%s(@%s)" % (self.key(), id(self))

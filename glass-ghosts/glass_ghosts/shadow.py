import InfiniteGlass, Xlib.X
import struct
import array
import pkg_resources
import sqlite3
import os.path
import json
import array
import base64
import glass_ghosts.helpers
import sys

class NoValue: pass

class Shadow(object):
    def __init__(self, manager, properties):
        self.manager = manager
        self._properties = {}
        self.properties = properties
        self.window = None
        self.current_key = None
        self.update_key()
        InfiniteGlass.DEBUG("shadow", "SHADOW CREATE %s\n" % (self,)); sys.stderr.flush()

    def key(self):
        return tuple(glass_ghosts.helpers.tuplify(self.properties.get(name, None)) for name in sorted(self.manager.MATCH))

    def update_key(self):
        key = self.key()

        if key != self.current_key and key in self.manager.shadows:
            InfiniteGlass.DEBUG("shadow", "DUPLICATE SHADOW %s\n" % (self,))
            self.destroy()
            
        if not self.manager.restoring_shadows:

            cur = self.manager.dbconn.cursor()
            dbkey = "/".join(str(item) for item in key)
            for name, value in self.properties.items():
                if self._properties.get(name, NoValue) != value:
                    cur.execute("""
                      insert or replace into shadows (key, name, value) VALUES (?, ?, ?)
                    """, (dbkey, name, json.dumps(value, default=self.manager.tojson)))
                    self.manager.changes = True
            for name, value in self._properties.items():
                if name not in self.properties:
                    cur.execute("""
                      delete from shadows where key=? and name=?
                    """, (dbkey, name))
                    self.manager.changes = True
            if key != self.current_key and self.current_key is not None:
                current_dbkey = "/".join(str(item) for item in self.current_key)
                try:
                    cur.execute("""
                      update shadows set key=? where key=?
                        """, (dbkey, current_dbkey))
                except Exception as e:
                    InfiniteGlass.DEBUG("shadow.database", "Error updating key in db: %s\nkey=%s, dbkey=%s  =>  key=%s dbkey=%s\n" % (
                        e, self.current_key, current_dbkey, key, dbkey))
                    self.destroy()
                self.manager.changes = True

        if key == self.current_key:
            return
        if self.current_key is not None:
            del self.manager.shadows[self.current_key]
            InfiniteGlass.DEBUG("shadow", "UPDATE KEY from %s to %s\n" % (self.current_key, key)); sys.stderr.flush()
        
        self.current_key = key
        self.manager.shadows[self.current_key] = self
            
    def apply(self, window):
        InfiniteGlass.DEBUG("shadow", "SHADOW APPLY window_id=%s %s\n" % (window.__window__(), self)); sys.stderr.flush()
        for key in self.manager.SET:
            if key in self.properties:
                window[key] = self.properties[key]
                
    def activate(self):
        InfiniteGlass.DEBUG("shadow", "SHADOW ACTIVATE %s\n" % (self,)); sys.stderr.flush()
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
                self.window["IG_CONTENT"]=("IG_SVG", ghost_image)
                self.window["WM_PROTOCOLS"] = ["WM_DELETE_WINDOW"]
        self.apply(self.window)

        @self.window.on(mask="StructureNotifyMask")
        def DestroyNotify(win, event):
            InfiniteGlass.DEBUG("shadow", "SHADOW DELETE %s\n" % (self,)); sys.stderr.flush()
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
                InfiniteGlass.DEBUG("shadow", "%s: Unknown WM_PROTOCOLS message: %s\n" % (self, event)); sys.stderr.flush()
        self.WMDelete = ClientMessage

        @self.window.on()
        def PropertyNotify(win, event):
            name = self.manager.display.get_atom_name(event.atom)
            try:
                self.properties.update(glass_ghosts.helpers.expand_property(win, name))
            except:
                pass
            else:
                self.update_key()

        @self.window.on()
        def ButtonPress(win, event):
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
        gcbg = self.manager.display.root.create_gc(foreground = self.manager.display.screen(0).white_pixel,
                                                   background = self.manager.display.screen(0).black_pixel)
        gcfg = self.manager.display.root.create_gc(foreground = self.manager.display.screen(0).black_pixel,
                                                   background = self.manager.display.screen(0).white_pixel)
        geom = self.window.get_geometry()
        self.window.fill_rectangle(gcbg, 0, 0, geom.width, geom.height)
        self.window.draw_text(gcfg, 10, 10, str(self.properties.get("WM_NAME", "")))
        self.window.draw_text(gcfg, 10, 30, str(self.properties.get("WM_CLASS", "")))
        self.manager.display.flush()
        
    def deactivate(self):
        InfiniteGlass.DEBUG("shadow", "SHADOW DEACTIVATE %s\n" % (self,)); sys.stderr.flush()
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

        self.manager.shadows.pop(self.key(), None)

        dbkey = str(self)
        cur = self.manager.dbconn.cursor()
        cur.execute("""
            delete from shadows where key = ?
        """, (dbkey,))
        self.manager.dbconn.commit()        
            
    def __str__(self):
        return "/".join(str(item) for item in self.key())

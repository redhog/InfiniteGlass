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

class Shadow(object):
    def __init__(self, manager, properties):
        self.manager = manager
        self.properties = properties
        self.window = None
        self.current_key = None
        self.update_key()
        print("SHADOW CREATE", self)

    def key(self):
        return tuple(glass_ghosts.helpers.tuplify(self.properties.get(name, None)) for name in sorted(self.manager.MATCH))

    def update_key(self):
        if not self.manager.restoring_shadows:
            cur = self.manager.dbconn.cursor()
            dbkey = str(self)
            for name, value in self.properties.items():
                cur.execute("""
                  insert or replace into shadows (key, name, value) VALUES (?, ?, ?)
                """, (dbkey, name, json.dumps(value, default=self.manager.tojson)))
                self.manager.dbconn.commit()
                
        key = self.key()
        if key == self.current_key:
            return
        if self.current_key is not None:
            del self.manager.shadows[self.current_key]
            print("UPDATE KEY from %s to %s" % (self.current_key, key))
            self.current_key = key
            self.manager.shadows[self.current_key] = self
            
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

        @self.window.on(mask="StructureNotifyMask")
        def DestroyNotify(win, event):
            print("SHADOW DELETE", self)
            self.manager.display.eventhandlers.remove(self.DestroyNotify)
            self.manager.display.eventhandlers.remove(self.PropertyNotify)
            self.manager.display.eventhandlers.remove(self.WMDelete)
            self.manager.shadows.pop(self.key(), None)

            dbkey = str(self)
            cur = self.manager.dbconn.cursor()
            cur.execute("""
                delete from shadows where key = ?
            """, (dbkey,))
            self.manager.dbconn.commit()
            
        @self.window.on(mask="NoEventMask", client_type="WM_PROTOCOLS")
        def ClientMessage(win, event):
            if event.parse("ATOM")[0] == "WM_DELETE_WINDOW":
                self.window.destroy()
            else:
                print("%s: Unknown WM_PROTOCOLS message: %s" % (self, event))

        @self.window.on()
        def PropertyNotify(win, event):
            name = self.manager.display.get_atom_name(event.atom)
            try:
                self.properties.update(glass_ghosts.helpers.expand_property(win, name))
            except:
                pass
            else:
                self.update_key()
                
        self.DestroyNotify = DestroyNotify
        self.WMDelete = ClientMessage
        self.PropertyNotify = PropertyNotify
                
        self.window.map()

    def deactivate(self):
        print("SHADOW DEACTIVATE", self)
        if self.window is not None:
            self.window.destroy()
            self.manager.display.eventhandlers.remove(self.DestroyNotify)
            self.manager.display.eventhandlers.remove(self.PropertyNotify)
            self.manager.display.eventhandlers.remove(self.WMDelete)
        
    def __str__(self):
        return "/".join(str(item) for item in self.key())

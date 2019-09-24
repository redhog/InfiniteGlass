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
import glass_ghosts.helpers
import sys

class Window(object):
    def __init__(self, manager, window):
        self.manager = manager
        self.window = window
        self.id = self.window.__window__()
        self.shadow = None
        self.client = None
        self.properties = {}
        for name in self.window.keys():
            self.properties.update(glass_ghosts.helpers.expand_property(self.window, name))
        sys.stderr.write("WINDOW CREATE %s\n" % (self,)); sys.stderr.flush()
        self.match()
            
        @window.on()
        def PropertyNotify(win, event):
            name = self.manager.display.get_atom_name(event.atom)
            try:
                self.properties.update(glass_ghosts.helpers.expand_property(win, name))
            except:
                pass
            else:
                self.match()

        @self.window.on(mask="StructureNotifyMask", client_type="IG_SLEEP")
        def ClientMessage(win, event):
            print("RECEIVED SLEEP", win, event, self.client)
            if self.client:
                self.client.conn.sleep()

        @window.on(mask="StructureNotifyMask")
        def DestroyNotify(win, event):
            sys.stderr.write("WINDOW DESTROY %s %s\n" % (self, event.window.__window__())); sys.stderr.flush()
            self.destroy()

        self.DestroyNotify = DestroyNotify
        self.SleepMessage = ClientMessage
        self.PropertyNotify = PropertyNotify
        
    def key(self):
        return tuple(glass_ghosts.helpers.tuplify(self.properties.get(name, None)) for name in sorted(self.manager.MATCH))

    def destroy(self):
        if not self.shadow:
            self.shadow = glass_ghosts.shadow.Shadow(self.manager, self.properties)
        else:
            self.shadow.properties.update(self.properties)
            self.shadow.update_key()
        self.shadow.activate()
        self.manager.windows.pop(self.id, None)
        self.manager.display.eventhandlers.remove(self.PropertyNotify)
        self.manager.display.eventhandlers.remove(self.SleepMessage)
        self.manager.display.eventhandlers.remove(self.DestroyNotify)
        if self.client and self.client.conn:
            self.client.conn.remove_window(self)
    
    def match(self):
        self.match_shadow()
        self.match_client()
        
    def match_shadow(self):
        if self.shadow: return
        key = self.key()
        if key in self.manager.shadows:
            self.shadow = self.manager.shadows[key]
            self.shadow.apply(self.window)
            self.shadow.deactivate()

    def match_client(self):
        if self.client: return
        if "SM_CLIENT_ID" not in self.properties: return
        client_id = self.properties["SM_CLIENT_ID"]
        if client_id not in self.manager.clients: return
        self.client = self.manager.clients[client_id]
        self.client.conn.add_window(self)
        
    def __str__(self):
        res = str(self.window.__window__())
        res += ": " + "/".join(str(item) for item in self.key())
        if self.shadow is not None:
            res += " (has shadow)"
        return res

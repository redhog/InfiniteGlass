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
        self.properties = {}
        for name in self.window.keys():
            self.properties.update(glass_ghosts.helpers.expand_property(self.window, name))
        sys.stderr.write("WINDOW CREATE %s\n" % (self,)); sys.stderr.flush()
        self.match_shadow()
            
        @window.on()
        def PropertyNotify(win, event):
            name = self.manager.display.get_atom_name(event.atom)
            try:
                self.properties.update(glass_ghosts.helpers.expand_property(win, name))
            except:
                pass
            else:
                self.match_shadow()
            
        @window.on(mask="StructureNotifyMask")
        def DestroyNotify(win, event):
            sys.stderr.write("WINDOW DESTROY %s %s\n" % (self, event.window.__window__())); sys.stderr.flush()
            if not self.shadow:
                self.shadow = glass_ghosts.shadow.Shadow(self.manager, self.properties)
            else:
                self.shadow.properties.update(self.properties)
                self.shadow.update_key()
            self.shadow.activate()
            self.manager.windows.pop(self.id, None)
            self.manager.display.eventhandlers.remove(PropertyNotify)
            self.manager.display.eventhandlers.remove(DestroyNotify)
            
    def key(self):
        return tuple(glass_ghosts.helpers.tuplify(self.properties.get(name, None)) for name in sorted(self.manager.MATCH))

    def match_shadow(self):
        if self.shadow: return
        key = self.key()
        if key in self.manager.shadows:
            self.shadow = self.manager.shadows[key]
            self.shadow.apply(self.window)
            self.shadow.deactivate()
        
    def __str__(self):
        res = str(self.window.__window__())
        res += ": " + "/".join(str(item) for item in self.key())
        if self.shadow is not None:
            res += " (has shadow)"
        return res

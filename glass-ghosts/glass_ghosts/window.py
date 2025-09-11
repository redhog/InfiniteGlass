import InfiniteGlass
import Xlib.X
import glass_ghosts.ghost
import glass_ghosts.helpers
import sys
import re

class Window(object):
    def __new__(cls, manager, window):
        self = object.__new__(cls)
        
        self.manager = manager
        self.window = window
        self.id = self.window.__window__()
        self.ghost = None
        self.client = None
        self.properties = {}

        self.override_redirect = self.window.get_attributes().override_redirect

        for name in self.window.keys():
            self.properties.update(glass_ghosts.helpers.expand_property(self.window, name))

        self.under_deletion = False
            
        if self.is_ignored():
            return None
        
        @self.window.on()
        def PropertyNotify(win, event):
            name = self.manager.display.get_atom_name(event.atom)
            try:
                self.properties.update(glass_ghosts.helpers.expand_property(win, name))
            except:
                pass
            else:
                # Make sure we don't match on a ghost before the renderer has had 
                if "WM_STATE" in self.properties:
                    self.match()
            InfiniteGlass.DEBUG("setprop", "%s.%s=%s\n" % (self.id, name, str(self.properties.get(name))[:400]))
        self.PropertyNotify = PropertyNotify

        @self.window.on(mask="StructureNotifyMask")
        def ConfigureNotify(win, event):
            config = {
                "x": event.x,
                "y": event.y,
                "width": event.width,
                "height": event.height
            }
            self.properties.update({"__config__": config})
            InfiniteGlass.DEBUG("setprop", "%s[%s]\n" % (self.id, config))
        self.ConfigureNotify = ConfigureNotify

        @self.window.on(mask="StructureNotifyMask", client_type="IG_SLEEP")
        def ClientMessage(win, event):
            InfiniteGlass.DEBUG("message", "RECEIVED SLEEP %s %s %s\n" % (win, event, self.client)); sys.stderr.flush()
            self.sleep()
        self.SleepMessage = ClientMessage

        @self.window.on(mask="StructureNotifyMask", client_type="IG_CLOSE")
        def ClientMessage(win, event):
            InfiniteGlass.DEBUG("message", "RECEIVED CLOSE %s %s %s\n" % (win, event, self.client)); sys.stderr.flush()
            self.close()
        self.CloseMessage = ClientMessage

        @self.window.on(mask="StructureNotifyMask", client_type="IG_DELETE")
        def ClientMessage(win, event):
            InfiniteGlass.DEBUG("message", "RECEIVED DELETE %s %s %s\n" % (win, event, self.client)); sys.stderr.flush()
            self.under_deletion = True
            self.close()
        self.DeleteMessage = ClientMessage

        @self.window.on(mask="StructureNotifyMask")
        def UnmapNotify(win, event):
            InfiniteGlass.DEBUG("window", "WINDOW UNMAP %s %s\n" % (self, event.window.__window__())); sys.stderr.flush()
            self.destroy()
        self.UnmapNotify = UnmapNotify

        InfiniteGlass.DEBUG("window", "WINDOW CREATE %s\n" % (self,)); sys.stderr.flush()

        self.match()

        return self
        
    def is_ignored(self):
        if self.override_redirect:
            return True
        if self.under_deletion:
            return True
        props = self.properties.keys()
        for ignore in self.manager.config["ignore"]:
            if isinstance(ignore, (tuple, list)):
                name, value = ignore
                if name in props:
                    propvals = self.properties[name]
                    if value == propvals or value in propvals:
                        return True
            elif ignore in props:
                return True
        return False
        
    def sleep(self):
        if self.client:
            for conn in self.client.connections.values():
                conn.sleep()
        else:
            self.close()

    def close(self):
        if self.client:
            if len(self.client.windows) <= 1:
                for conn in self.client.connections.values():
                    conn.sleep()
                return
        if "WM_PROTOCOLS" in self.window and "WM_DELETE_WINDOW" in self.window["WM_PROTOCOLS"]:
            self.window.send(self.window, "WM_PROTOCOLS", "WM_DELETE_WINDOW", Xlib.X.CurrentTime)
        else:
            self.window.destroy()

    def key(self):
        return glass_ghosts.helpers.ghost_key(self.properties, self.manager.config["match"])

    def destroy(self):
        InfiniteGlass.DEBUG("destroy", "Window destroyed %s" % (self.window.get("WM_NAME", self.window),))
        InfiniteGlass.DEBUG("destroy", "  key=%s" % (self.key(),))
        InfiniteGlass.DEBUG("destroy", "  %s" % ("IGNORED" if self.is_ignored() else "NOT IGNORED",))
        InfiniteGlass.DEBUG("destroy", "  %s" % ("has ghost" if self.ghost else "no ghost",))
        
        if not self.is_ignored():
            if not self.ghost:
                self.ghost = glass_ghosts.ghost.Shadow(self.manager, self.properties)
            else:
                self.ghost.properties.update(self.properties)
                self.ghost.update_key()
            self.ghost.activate()
        else:
            if self.ghost:
                self.ghost.destroy()
        self.manager.windows.pop(self.id, None)
        self.manager.display.eventhandlers.remove(self.PropertyNotify)
        self.manager.display.eventhandlers.remove(self.DeleteMessage)
        self.manager.display.eventhandlers.remove(self.CloseMessage)
        self.manager.display.eventhandlers.remove(self.SleepMessage)
        self.manager.display.eventhandlers.remove(self.UnmapNotify)
        self.manager.display.eventhandlers.remove(self.ConfigureNotify)
        if self.client:
            self.client.remove_window(self)

    def match(self):
        self.match_ghost()
        self.match_client()

    def match_ghost(self):
        if self.ghost: return
        key = self.key()
        ghost = None
        if key in self.manager.ghosts:
            ghost = self.manager.ghosts[key]
        else:
            if "SM_CLIENT_ID" in self.properties and self.properties["SM_CLIENT_ID"] in self.manager.clients:
                client = self.manager.clients[self.properties["SM_CLIENT_ID"]]
                if len(client.ghosts) == 1:
                    ghost = list(client.ghosts.values())[0]
        if ghost is not None and not ghost.is_active:
            ghost = None
        if ghost is not None:
            self.ghost = ghost
            InfiniteGlass.DEBUG("window", "MATCHING SHADOW window=%s ghost=%s\n" % (self.id, key,))
            self.ghost.apply(self.window)
            self.ghost.deactivate()
        else:
            InfiniteGlass.DEBUG("window", "FAILED MATCHING window=%s key=%s against SHADOWS %s\n" % (self.id, key, self.manager.ghosts.keys()))
            if "IG_TEMPLATE_APPLIED" not in self.window:
                for pattern, template in self.manager.config.get("templates", {}).items():
                    if re.match(pattern, key):
                        self.window["IG_TEMPLATE_APPLIED"] = pattern.encode("UTF-8")
                        InfiniteGlass.DEBUG("window", "MATCHED window=%s key=%s against TEMPLATE %s\n" % (self.id, key, pattern))
                        for name, value in template.items():
                            self.window[name] = value
                        self.manager.display.flush()
                        break
            
    def match_client(self):
        if self.client: return
        if "SM_CLIENT_ID" not in self.properties: return
        client_id = self.properties["SM_CLIENT_ID"]
        if client_id not in self.manager.clients: return
        InfiniteGlass.DEBUG("window", "MATCH CLIENT window=%s client_id=%s\n" % (self.id, client_id))
        sys.stderr.flush()
        self.client = self.manager.clients[client_id]
        self.client.add_window(self)

    def __str__(self):
        res = str(self.window.__window__())
        res += ": " + self.key()
        if self.ghost is not None:
            res += " (has ghost)"
        return res

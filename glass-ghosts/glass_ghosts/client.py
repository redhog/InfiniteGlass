import InfiniteGlass
import os.path
import json
import uuid

class NoValue(object): pass

class Client(object):
    def __init__(self, manager, client_id=None, properties=None):
        self.manager = manager
        self.client_id = client_id or uuid.uuid4().hex.encode("ascii")
        self._properties = {}
        self.properties = properties or {}
        self.updatedb()
        self.windows = {}
        self.shadows = {}
        self.connections = {}

    def add_window(self, window):
        self.windows[window.id] = window

    def remove_window(self, window):
        self.windows.pop(window.id)

    def add_connection(self, conn):
        self.connections[conn.fd] = conn

    def remove_connection(self, conn):
        self.connections.pop(conn.fd)

    def __getitem__(self, name):
        return self.properties[name]
    def __setitem__(self, name, value):
        self.properties[name] = value
        self.updatedb()
    def __delitem__(self, name):
        del self.properties[name]
        self.updatedb()
    def update(self, props):
        self.properties.update(props)
        self.updatedb()

    def updatedb(self):
        if self.manager.restoring_clients: return

        cur = self.manager.dbconn.cursor()
        for name, value in self.properties.items():
            if self._properties.get(name, NoValue) != value:
                cur.execute("""
                  insert or replace into clients (key, name, value) VALUES (?, ?, ?)
                """, (self.client_id, name, json.dumps(value, default=InfiniteGlass.tojson(self.manager.display))))
                self.manager.changes = True
        for name, value in self._properties.items():
            if name not in self.properties:
                cur.execute("""
                  delete from clients where key=? and name=?
                """, (self.client_id, name))
                self.manager.changes = True

    def restart(self):
        assert "RestartCommand" in self.properties, "Session client did not provide a restart command"
        InfiniteGlass.DEBUG("restart", "Restarting %s by running %s\n" % (self.client_id, " ".join(self.properties["RestartCommand"][1])))

        env = dict(os.environ)
        env["SESSION_MANAGER"] = self.manager.session.listen_address()

        if os.fork() == 0:
            os.execlpe(self.properties["RestartCommand"][1][0], *self.properties["RestartCommand"][1], env)

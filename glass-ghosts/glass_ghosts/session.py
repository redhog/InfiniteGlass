import pysmlib.server
import pysmlib.ice
import pysmlib.iceauth
import select
import uuid

class Server(pysmlib.server.Server):
    def __init__(self, display, manager):
        self.display = display
        self.manager = manager
        pysmlib.server.Server.__init__(self)

        self.listeners = self.IceListenForConnections()
        pysmlib.iceauth.SetAuthentication(self.listeners)

        for listener in self.listeners:
            self.display.mainloop.add(listener.IceGetListenConnectionNumber(), lambda fd: listener.IceAcceptConnection())
    
    class Connection(pysmlib.server.PySmsConn):
        def __init__(self, *arg, **kw):
            pysmlib.server.PySmsConn.__init__(self, *arg, **kw)
            self.fd = self.iceconn.IceConnectionNumber()
            self.manager.display.mainloop.add(self.fd, lambda fd: self.iceconn.IceProcessMessages())
            
        def register_client(self, *arg, **kw):
            print("register_client", arg, kw)
            self.id = uuid.uuid4().hex.encode("ascii")
            self.SmsRegisterClientReply(self.id)
            self.SmsSaveYourself(pysmlib.server.SmSaveGlobal, 0, pysmlib.server.SmInteractStyleAny, 1)
            return 1

        def interact_request(self, *arg, **kw):
            print("interact_request", arg, kw)

        def interact_done(self, *arg, **kw):
            print("interact_done", arg, kw)

        def save_yourself_request(self, *arg, **kw):
            for conn in self.manager.connections.values():
                if conn != self:
                    conn.SmsSaveYourself(pysmlib.server.SmSaveGlobal, 0, pysmlib.server.SmInteractStyleAny, 1)
                    
            print("save_yourself_request", arg, kw)

        def save_yourself_phase2_request(self, *arg, **kw):
            print("save_yourself_phase2_request", arg, kw)

        def save_yourself_done(self, *arg, **kw):
            print("save_yourself_done", arg, kw)

        def close_connection(self, *arg, **kw):
            print("close_connection", arg, kw)

        def set_properties(self, *arg, **kw):
            print("set_properties", arg, kw)

        def delete_properties(self, *arg, **kw):
            print("delete_properties", arg, kw)

        def get_properties(self, *arg, **kw):
            print("get_properties", arg, kw)

        def ice_ping_reply(self, *arg, **kw):
            print("ice_ping_reply", arg, kw)

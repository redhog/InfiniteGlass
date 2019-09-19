import pysmlib.server
import pysmlib.ice
import pysmlib.iceauth
import select
import uuid
import glass_ghosts.client
import sys

class Server(pysmlib.server.Server):
    def __init__(self, display, manager):
        self.display = display
        self.manager = manager
        pysmlib.server.Server.__init__(self)

        self.clients = {}
        
        self.listeners = self.IceListenForConnections()
        pysmlib.iceauth.SetAuthentication(self.listeners)

        def accepter(listener):
            print("LISTENING TO", listener, listener.IceGetListenConnectionNumber())
            sys.stdout.flush()
            self.display.mainloop.add(listener.IceGetListenConnectionNumber(), lambda fd: self.accept_connection(listener))
            
        for listener in self.listeners:
            accepter(listener)

    def accept_connection(self, listener):
        print("ACCEPTING CONNECTION FROM", listener)
        sys.stdout.flush()
        conn = listener.IceAcceptConnection()
        print("ACCEPTED CONNECTION", listener, conn)
        sys.stdout.flush()
        print("ACCEPTED CONNECTION", conn, conn.IceConnectionNumber())
        sys.stdout.flush()
        def process(fd):
            print("PROCESS", fd, conn)
            sys.stdout.flush()
            conn.IceProcessMessages()
        self.display.mainloop.add(conn.IceConnectionNumber(), process) #lambda fd: conn.IceProcessMessages())
             
    class Connection(pysmlib.server.PySmsConn):
        def __init__(self, *arg, **kw):
            pysmlib.server.PySmsConn.__init__(self, *arg, **kw)
            self.fd = self.SmsGetIceConnection().IceConnectionNumber()
            
        def register_client(self, previous_id):
            print("register_client", previous_id)
            sys.stdout.flush()
            if previous_id is not None and previous_id in self.clients:
                client = self.clients[previous_id]
            else:
                client = glass_ghosts.client.Client(self, previous_id)
                self.manager.clients[client.client_id] = client            
            self.client = client
            print("REGISTER DONE")
            sys.stdout.flush()
            self.SmsRegisterClientReply(self.client.client_id)
            return 1

        def interact_request(self, *arg, **kw):
            print("interact_request", arg, kw)

        def interact_done(self, *arg, **kw):
            print("interact_done", arg, kw)

        def save_yourself_request(self, *arg, **kw):
            for conn in self.manager.connections.values():
                if conn != self:
                    conn.SmsSaveYourself(pysmlib.server.SmSaveGlobal, 0, pysmlib.server.SmInteractStyleAny, 1)

        def save_yourself_phase2_request(self, *arg, **kw):
            print("save_yourself_phase2_request", arg, kw)

        def save_yourself_done(self, *arg, **kw):
            print("save_yourself_done", arg, kw)

        def close_connection(self, *arg, **kw):
            print("close_connection", arg, kw)
            self.manager.display.mainloop.remove(self.fd)

        def set_properties(self, props):
            self.client.update(props)

        def delete_properties(self, names):
            for name in names:
                del self.client[name]

        def get_properties(self):
            return self.client.properties

        def ice_ping_reply(self, *arg, **kw):
            print("ice_ping_reply", arg, kw)

    def listen_address(self):
        return ",".join(listener.IceGetListenConnectionString().decode("utf-8")
                        for listener in self.listeners)
    

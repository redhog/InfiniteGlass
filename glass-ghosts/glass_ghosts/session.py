import pysmlib.server
import pysmlib.ice
import pysmlib.iceauth
import select
import uuid
import glass_ghosts.client
import sys

class Server(pysmlib.server.Server):
    def __init__(self, manager, display):
        self.display = display
        self.manager = manager
        pysmlib.server.Server.__init__(self)
        
        self.listeners = self.IceListenForConnections()
        pysmlib.iceauth.SetAuthentication(self.listeners)

        def accepter(listener):
            sys.stderr.write("LISTENING TO %s @ %s\n" % (listener, listener.IceGetListenConnectionNumber()))
            sys.stderr.flush()
            self.display.mainloop.add(listener.IceGetListenConnectionNumber(), lambda fd: self.accept_connection(listener))
            
        for listener in self.listeners:
            accepter(listener)

    def accept_connection(self, listener):
        conn = listener.IceAcceptConnection()
        sys.stderr.write("ACCEPTED CONNECTION %s %s\n" % (listener, conn))
        sys.stderr.flush()
        def process(fd):
            #sys.stderr.write("PROCESS %s %s\n" % (fd, conn))
            #sys.stderr.flush()
            try:
                conn.IceProcessMessages()
            except Exception as e:
                print(e)
                self.display.mainloop.remove(conn.IceConnectionNumber())
        self.display.mainloop.add(conn.IceConnectionNumber(), process) #lambda fd: conn.IceProcessMessages())
             
    class Connection(pysmlib.server.PySmsConn):
        def __init__(self, *arg, **kw):
            self.client = None
            pysmlib.server.PySmsConn.__init__(self, *arg, **kw)
            self.ice_conn = self.SmsGetIceConnection()
            self.ice_conn.error_handler = self.error_handler
            self.ice_conn.io_error_handler = self.io_error_handler
            self.fd = self.ice_conn.IceConnectionNumber()
            self.do_sleep = False
            
        def error_handler(self, swap, offendingMinorOpcode, offendingSequence, errorClass, severity):
            sys.stderr.write("Error: %s: swap=%s, offendingMinorOpcode=%s, offendingSequence=%s, errorClass=%s, severity=%s)\n" %
                             (self, swap, offendingMinorOpcode, offendingSequence, errorClass, severity))
            sys.stderr.flush()
            self.close_connection()

        def io_error_handler(self):
            sys.stderr.write("IO Error: %s\n" % (self,))
            sys.stderr.flush()
            self.close_connection()

        def sleep(self):
            self.do_sleep = True
            self.SmsSaveYourself(pysmlib.server.SmSaveBoth, False, pysmlib.server.SmInteractStyleAny, False)
            
        def register_client(self, previous_id):
            sys.stderr.write("register_client client_id=%s\n" % (previous_id,))
            sys.stderr.flush()
            if previous_id is not None and previous_id in self.manager.manager.clients:
                client = self.manager.manager.clients[previous_id]
            else:
                print("REGISTERING", previous_id)
                client = glass_ghosts.client.Client(self.manager.manager, previous_id)
                self.manager.manager.clients[client.client_id] = client            
            self.client = client
            self.client.add_connection(self)
            sys.stderr.write("REGISTER DONE fd=%s client_id=%s\n" % (self.fd, client.client_id))
            sys.stderr.flush()
            self.SmsRegisterClientReply(self.client.client_id)
            return 1

        def interact_request(self, *arg, **kw):
            sys.stderr.write("interact_request %s %s\n" % (arg, kw))
            sys.stderr.flush()

        def interact_done(self, *arg, **kw):
            sys.stderr.write("interact_done %s %s\n" % (arg, kw))
            sys.stderr.flush()

        def save_yourself_request(self, *arg, **kw):
            for conn in self.manager.connections.values():
                if conn != self:
                    conn.SmsSaveYourself(pysmlib.server.SmSaveGlobal, 0, pysmlib.server.SmInteractStyleAny, 1)

        def save_yourself_phase2_request(self, *arg, **kw):
            print("save_yourself_phase2_request %s %s\n" % (arg, kw))
            sys.stderr.flush()

        def save_yourself_done(self, *arg, **kw):
            print("save_yourself_done %s %s\n" % (arg, kw))

            if self.do_sleep:
                self.SmsDie()
            
            sys.stderr.flush()

        def close_connection(self, *arg, **kw):
            print("close_connection %s %s\n" % (arg, kw))
            sys.stderr.flush()
            self.manager.display.mainloop.remove(self.fd)
            if self.client:
                self.client.remove_connection(self)

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
    

import pysmlib.server
import pysmlib.ice
import iceauth
import select

pysmlib.ice.PyIceSetPaAuthData([])

class Server(pysmlib.server.Server):
    class Connection(pysmlib.server.PySmsConn):
    
        def register_client(self, *arg, **kw):
            print("register_client", arg, kw)
            return 1

        def interact_request(self, *arg, **kw):
            print("interact_request", arg, kw)

        def interact_done(self, *arg, **kw):
            print("interact_done", arg, kw)

        def save_yourself_request(self, *arg, **kw):
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

server = Server()
conns = server.IceListenForConnections()
print("Listening to %s" % (",".join(conn.IceGetListenConnectionString().decode("utf-8") for conn in conns)))
iceauth.SetAuthentication(conns)

listeners = {conn.IceGetListenConnectionNumber(): conn for conn in conns}


while True:
    rlist, wlist, xlist = select.select(listeners.keys(), [], [])

    for fd in rlist:
        listener = listeners[fd]
        
        if hasattr(listener, "IceAcceptConnection"):
            conn = listener.IceAcceptConnection()
            listeners[conn.IceConnectionNumber()] = conn
        elif hasattr(listener, "IceProcessMessages"):
            try:
                listener.IceProcessMessages()
            except:
                listener.IceCloseConnection()

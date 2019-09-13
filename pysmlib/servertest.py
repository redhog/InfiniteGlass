import pysmlib.server

class Server(pysmlib.server.Server):
    class Connection(pysmlib.server.Connection):
    
        def register_client(self, *arg, **kw):
            pass

        def interact_request(self, *arg, **kw):
            pass

        def interact_done(self, *arg, **kw):
            pass

        def save_yourself_request(self, *arg, **kw):
            pass

        def save_yourself_phase2_request(self, *arg, **kw):
            pass

        def save_yourself_done(self, *arg, **kw):
            pass

        def close_connection(self, *arg, **kw):
            pass

        def set_properties(self, *arg, **kw):
            pass

        def delete_properties(self, *arg, **kw):
            pass

        def get_properties(self, *arg, **kw):
            pass

        def ice_ping_reply(self, *arg, **kw):
            pass

Server()
print("Server done")

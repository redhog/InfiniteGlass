import pysmlib.client

class MyConnection(pysmlib.client.PySmcConn):
    def signal_save_yourself(self, *arg):
        print("SAVE_YOURSELF", arg)
        self.save_yourself_done()
    def signal_die(self, *arg):
        print("DIE", arg)
    def signal_save_complete(self, *arg):
        print("SAVE_COMPLETE", arg)
    def signal_shutdown_cancelled(self, *arg):
        print("SHUTDOWN_CANCELLED", arg)

c = MyConnection()
print("Connection established. ID: ", c.client_id)
try:
    while True:
        c.iceconn.IceProcessMessages()
finally:
    c.SmcCloseConnection()

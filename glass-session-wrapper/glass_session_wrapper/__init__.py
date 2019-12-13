import InfiniteGlass
import Xlib.X
import sys
import os
import pysmlib.client
import socket
import signal

class MyConnection(pysmlib.client.PySmcConn):
    def __init__(self, wrapper):
        self.wrapper = wrapper
        pysmlib.client.PySmcConn.__init__(self)
    def signal_save_yourself(self, *arg):
        print("SAVE_YOURSELF", arg)
        print("Restart command: %s" % (["glass-session-wrapper"] + self.wrapper.argv,))
        self.SmcSetProperties({
            "RestartCommand": ["glass-session-wrapper"] + self.wrapper.argv,
            "SmCurrentDirectory": os.getcwd()
        })
        self.SmcSaveYourselfDone(True)
    def signal_die(self, *arg):
        print("DIE", arg)
        os.kill(self.wrapper.pid, signal.SIGINT)
    def signal_save_complete(self, *arg):
        print("SAVE_COMPLETE", arg)
    def signal_shutdown_cancelled(self, *arg):
        print("SHUTDOWN_CANCELLED", arg)

class Wrapper(object):
    def __init__(self, argv):
        self.argv = argv

        self.status_r, self.status_w = os.pipe()
        signal.signal(
            signal.SIGCHLD,
            self.sig_child)
        
        self.machine = socket.gethostname()
        
        self.pid = os.fork()
        if self.pid == 0:
            os.close(self.status_r)
            os.close(self.status_w)
            os.execvp(argv[0], argv)
            os._exit(127) # This shouldn't happen...
        else:
            with InfiniteGlass.Display() as self.display:
                self.display.mainloop.add(self.status_r, self.child_done)
                self.conn = MyConnection(self)
                self.display.mainloop.add(self.conn.iceconn.IceConnectionNumber(), lambda fd: self.conn.iceconn.IceProcessMessages())
                self.client_id = self.conn.client_id.decode("utf-8")
                print("Session ID: ", self.client_id)

                @self.display.root.on(mask="SubstructureNotifyMask")
                def MapNotify(win, event):
                    self.match(event.window)

    def match(self, win):
        win = win.find_client_window()
        if not win: return
        machine = win.get("WM_CLIENT_MACHINE", b"").decode("utf-8")
        pid = win.get("_NET_WM_PID", None)
        name = win.get("WM_NAME", "").decode("utf-8")
        print("MATCHING %s: %s/%s against %s/%s" % (name, machine, pid, self.machine, self.pid))
        if pid == self.pid and machine == self.machine:
            print("Found matching window %s" % win)
            win["SM_CLIENT_ID"] = self.client_id.encode("utf-8")

    def sig_child(self, sig, frame):
        print("SIG CHILD")
        pid, self.status = os.waitpid(self.pid, os.WNOHANG)
        if pid != 0:
            os.write(self.status_w, b"x")
            os.close(self.status_w)
                        
    def child_done(self, fd):
        print("Child done according to file descriptor"); sys.stdout.flush()
        os.close(self.status_r)
        print("Closed status_r"); sys.stdout.flush()
        print("Waitpid returned %s" % self.status); sys.stdout.flush()
        self.conn.SmcCloseConnection()
        print("SmcConnection closed"); sys.stdout.flush()
        sys.exit(self.status)
        
def main(*arg, **kw):
    Wrapper(sys.argv[1:])
    

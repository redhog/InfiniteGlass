import InfiniteGlass.debug
import Xlib.X
import glass_ghosts.ghost
import glass_ghosts.window
import json
import os
import signal
import traceback

class Components(object):
    def __init__(self, manager, display):
        self.manager = manager
        self.display = display
        self.components = {}
        self.components_by_pid = {}
        
        self.old_sigchld = signal.signal(signal.SIGCHLD, self.sigchild)
        
        @display.root.on()
        def PropertyNotify(win, event):
            name = self.manager.display.get_atom_name(event.atom)
            if name.startswith("IG_COMPONENT_"):
                spec = json.loads(display.root[name].decode("utf-8"))
                self.start_component(spec)

        for name, spec in self.manager.config.get("components", {}).items():
            spec["name"] = name
            display.root["IG_COMPONENT_" + name] = json.dumps(spec).encode("utf-8")

            
    def sigchild(self, signum, frame):
        try:
            if signum == signal.SIGCHLD:
                InfiniteGlass.debug.DEBUG("SIGCHLD", "Received SIGCHLD\n")
                try:
                    pid, exitcode, ru_child = os.wait4(-1, os.WNOHANG)
                except ChildProcessError:
                    pid = 0
                while pid != 0:
                    InfiniteGlass.debug.DEBUG("SIGCHLD", "Reaped pid=%s, exitcode=%s, ru_child=%s\n" % (pid, exitcode, ru_child))
                    if pid in self.components_by_pid:
                        name = self.components_by_pid.pop(pid)
                        del self.display.root["IG_COMPONENTPID_" + name]
                        if (    not exitcode == 0
                            and (   not os.WIFSIGNALED(exitcode)
                                 or os.WTERMSIG(exitcode) not in (signal.SIGHUP, signal.SIGINT, signal.SIGQUIT, signal.SIGKILL))):
                            # This is a component, and it wasn't killed intentionally... restart it
                            if name in self.components:
                                spec = self.components[name]["component"]
                                self.start_component(spec)
                    InfiniteGlass.debug.DEBUG("SIGCHLD", "Checking for more children in the same batch...\n")
                    try:
                        pid, exitcode, ru_child = os.wait4(-1, os.WNOHANG)
                    except ChildProcessError:
                        pid = 0
                InfiniteGlass.debug.DEBUG("SIGCHLD", "Done\n")
        except Exception as e:
            print(e)
            traceback.print_exc()
                
    def start_component(self, spec):
        name = spec["name"]
        InfiniteGlass.debug.DEBUG("component", "Updating %s\n" % (name,))
        if name in self.components:
            pid = self.components[name]["pid"]
            existing_name = self.components_by_pid.pop(pid, None)
            if existing_name is not None:
                try:
                    os.kill(pid, signal.SIGQUIT)
                except ProcessLookupError:
                    InfiniteGlass.debug.DEBUG("component", "Old process had died unnoticed.\n")
        if not spec.get("run", True):
            InfiniteGlass.debug.DEBUG("component", "Stopped %s\n" % (name,))
            return
        InfiniteGlass.debug.DEBUG("component", "Starting %s: %s\n" % (name, " ".join(spec["command"])))
        pid = os.fork()
        if pid == 0:
            try:
                os.execlp(spec["command"][0], *spec["command"])
            except Exception as e:
                raise Exception("%s (%s): %s" % (name, " ".join(spec["command"]), e))
        else:
            self.components[name] = {"pid": pid, "component": spec}
            self.components_by_pid[pid] = name
            self.display.root["IG_COMPONENTPID_" + name] = pid
        

import Xlib.X
import glass_ghosts.shadow
import glass_ghosts.window
import json
import os
import signal


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
        if signum == signal.SIGCHLD:
            pid, exitcode, ru_child = os.wait4(-1, os.WNOHANG)
            while pid != 0:
                if (    pid in self.components_by_pid
                    and not exitcode == 0
                    and (   not os.WIFSIGNALED(exitcode)
                         or os.WTERMSIG(exitcode) not in (signal.SIGHUP, signal.SIGINT, signal.SIGQUIT, signal.SIGKILL))):
                    # This is a component, and it wasn't killed intentionally... restart it
                    name = self.components_by_pid.pop(pid)
                    spec = self.components.pop(name)["component"]
                    self.start_component(spec)
                pid, exitcode, ru_child = os.wait4(-1, os.WNOHANG)
            
    def start_component(self, spec):
        print("Starting %s" % json.dumps(spec))
        if spec["name"] in self.components:
            pid = self.components[spec["name"]]["pid"]
            del self.components_by_pid[pid]
            os.kill(pid, signal.SIGINT)
        pid = os.fork()
        if pid == 0:
            os.execlp(spec["command"][0], *spec["command"])
        else:
            self.components[spec["name"]] = {"pid": pid, "component": spec}
            self.components_by_pid[pid] = spec["name"]


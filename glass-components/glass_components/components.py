import InfiniteGlass.debug
import Xlib.X
import glass_ghosts.ghost
import glass_ghosts.window
import json
import yaml
import os
import signal
import traceback
import time

class Components(object):
    def __init__(self, display, **kw):
        self.display = display

        configpath = os.path.expanduser(os.environ.get("GLASS_GHOSTS_CONFIG", "~/.config/glass/components.yml"))
        with open(configpath) as f:
            self.config = InfiniteGlass.load_yaml(f, self.display)

        self.components = {}
        self.components_by_pid = {}
        self.restart_components = self.config.get("restart_components", True)
        
        self.old_sigchld = signal.signal(signal.SIGCHLD, self.sigchild)
        
        @display.root.on()
        def PropertyNotify(win, event):
            name = self.display.get_atom_name(event.atom)
            if name.startswith("IG_COMPONENT_"):
                spec = json.loads(display.root[name].decode("utf-8"))
                self.start_component(spec)

        for name, spec in self.config.get("components", {}).items():
            spec["name"] = name
            display.root["IG_COMPONENT_" + name] = json.dumps(spec).encode("utf-8")

    def shutdown(self):
        InfiniteGlass.DEBUG("shutdown", "Shutting down components\n")
        for i in range(5):
            for pid, name in list(self.components_by_pid.items()):
                InfiniteGlass.DEBUG("shutdown", "Shutting down component %s (%s)\n" % (name, pid))
                try:
                    os.kill(pid, signal.SIGQUIT)
                except ProcessLookupError:
                    try:
                        del self.components_by_pid[pid]
                    except:
                        pass
                    InfiniteGlass.debug.DEBUG("component", "Old process had died unnoticed.\n")
            if not self.components_by_pid:
                break
            time.sleep(1)
            
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
                        
                        if name in self.components:
                            spec = self.components[name]["component"]
                            spec_actions = spec.get("exit_actions", {})
                            default_actions = self.config.get("defaults", {}).get("exit_actions", {})

                            exit_action = spec_actions.get("exited", default_actions.get("exited", "nothing"))
                            killed_action = spec_actions.get("killed", default_actions.get("killed", "nothing"))
                            fail_action = spec_actions.get("failed", default_actions.get("failed", "restart"))

                            if exitcode == 0:
                                action = exit_action
                            elif (os.WIFSIGNALED(exitcode)
                                  and os.WTERMSIG(exitcode) in (signal.SIGHUP, signal.SIGINT, signal.SIGQUIT, signal.SIGKILL)):
                                action = killed_action
                            else:
                                action = fail_action
                            
                            if action == "restart":
                                self.start_component(spec)
                            elif action == "exit":
                                self.display.exit()
                                return

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
            env = dict(os.environ)
            if "environment" in spec:
                env.update(spec["environment"])
            try:
                os.execvpe(spec["command"][0], spec["command"], env)
            except Exception as e:
                raise Exception("%s (%s): %s" % (name, " ".join(spec["command"]), e))
        else:
            self.components[name] = {"pid": pid, "component": spec}
            self.components_by_pid[pid] = name
            self.display.root["IG_COMPONENTPID_" + name] = pid
        

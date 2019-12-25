import Xlib.X
import glass_ghosts.shadow
import glass_ghosts.window
import json
import os

class Components(object):
    def __init__(self, manager, display):
        self.manager = manager
        self.display = display
        self.components = {}
        
        @display.root.on()
        def PropertyNotify(win, event):
            name = self.manager.display.get_atom_name(event.atom)
            if name.startswith("IG_COMPONENT_"):
                spec = json.loads(display.root[name].decode("utf-8"))
                self.start_component(spec)

        for name, spec in self.manager.config.get("components", {}).items():
            spec["name"] = name
            display.root["IG_COMPONENT_" + name] = json.dumps(spec).encode("utf-8")

    def start_component(self, spec):
        print("Starting %s" % json.dumps(spec))
        if spec["name"] in self.components:
            os.kill(self.components[spec["name"]]["pid"], signal.SIGINT)
        pid = os.fork()
        if pid == 0:
            os.execlp(spec["command"][0], *spec["command"])
        else:
            self.components[spec["name"]] = {"pid": pid, "component": spec}

    

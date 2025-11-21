import importlib.metadata
from . import debug

class ActionRunner(object):
    def __init__(self):
        self.actions = {entry_point.name: entry_point.load()
                        for entry_point
                        in importlib.metadata.entry_points(group="InfiniteGlass.actions")}
        
    def call_action(self, action, args, name=None, **kw):
        if action in self.actions:
            debug.DEBUG("final_action", "Function call %s(%s)\n" % (action, args))
            self.actions[action](self, **args, **kw)
        else:
            raise Exception("Unknown action for %s: %s\n" % (name, action))
    
    def run(self, action, name=None, **kw):
        name = name or action
        debug.DEBUG("action", "Action %s.%s\n" % (self, name))
        if isinstance(action, (tuple, list)):
            for item in action:
                run(item, **kw)
        else:
            args = {}
            if isinstance(action, dict):
                args = next(iter(action.values()))
                action = next(iter(action.keys()))
                if not isinstance(args, dict):
                    args = {"value": args}

            if not isinstance(action, str):
                raise Exception("Unknown action type for %s: %s\n" % (name, action,))

            self.call_action(action, args, name=name, **kw)

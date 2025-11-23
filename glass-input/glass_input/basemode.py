import InfiniteGlass.windows
import InfiniteGlass.action

class BaseMode(InfiniteGlass.action.ActionRunner):
    def __init__(self, display, **kw):
        self.first_event = None
        self.last_event = None
        self.state = {}
        InfiniteGlass.action.ActionRunner.__init__(self, display, **kw)

    def get_window(self, event=None, **kw):
        event = event or self.last_event
        if event == "ClientMessage":
            return event.window
        return InfiniteGlass.windows.get_event_window(self.display, event)

    def __getitem__(self, name):
        if name in self.state:
            return self.state[name]
        if hasattr(self, name):
            return getattr(self, name)
        raise KeyError

    @property
    def last_event_window(self):
        return self.get_window(self.last_event)

    def __repr__(self):
        if hasattr(self, "name"):            
            return "%s/%s" % (type(self).__name__, self.name)
        return type(self).__name__

import InfiniteGlass
import glass_ghosts.manager
import glass_ghosts.session

class Manager(object):
    def __init__(self, display):
        self.ghosts = glass_ghosts.manager.GhostManager(display)
        self.session = glass_ghosts.session.Server(display, self)

    
with InfiniteGlass.Display() as display:
    manager = Manager(display)
    print("Session manager listening to %s" % (
        ",".join(listener.IceGetListenConnectionString().decode("utf-8")
                 for listener in manager.session.listeners)))

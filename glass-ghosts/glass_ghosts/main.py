import InfiniteGlass
import glass_ghosts.manager
import glass_ghosts.session
import sys

class Manager(object):
    def __init__(self, display):
        self.ghosts = glass_ghosts.manager.GhostManager(display)
        self.session = glass_ghosts.session.Server(display, self)

    
with InfiniteGlass.Display() as display:
    manager = Manager(display)
    sys.stdout.write("%s\n" % manager.session.listen_address())
    sys.stdout.flush()
    sys.stderr.write("Session manager listening to %s\n" % manager.session.listen_address())
    sys.stderr.flush()
    

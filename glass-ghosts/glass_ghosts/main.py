import InfiniteGlass
import glass_ghosts.manager
import glass_ghosts.session

with InfiniteGlass.Display() as display:
    manager = glass_ghosts.manager.GhostManager(display)
    session = glass_ghosts.session.Server(display, manager)
    print("Session manager listening to %s" % (",".join(listener.IceGetListenConnectionString().decode("utf-8") for listener in session.listeners)))

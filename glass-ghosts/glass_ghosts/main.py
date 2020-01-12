import InfiniteGlass
import glass_ghosts.manager
import glass_ghosts.session
import sys
import traceback
import signal
import os

def main2():
    manager = None
    try:
        with InfiniteGlass.Display() as display:
            manager = glass_ghosts.manager.GhostManager(display)
            sys.stdout.write("%s\n" % manager.session.listen_address())
            sys.stdout.flush()
            InfiniteGlass.DEBUG("init", "Session manager listening to %s\n" % manager.session.listen_address())
    except Exception as e:
        print("Ghost manager systemic failure, restarting: %s" % (e,))
        traceback.print_exc()
        try:
            if manager is not None and hasattr(manager, "components") and hasattr(manager.components, "components_by_pid"):
                for pid in manager.components.components_by_pid.keys():
                    os.kill(pid, signal.SIGINT)
        except Exception as e:
            print(e)
            traceback.print_exc()
        os.execlp(sys.argv[0], *sys.argv)        
    print("END")

def main():
    import cProfile
    cProfile.runctx('main2()', globals(), locals(), "prof.prof")

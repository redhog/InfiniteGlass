import InfiniteGlass
import glass_session.manager
import sys
import traceback
import signal
import os

if os.environ.get("GLASS_DEBUGGER", "") == "rpdb":
    import rpdb
    rpdb.handle_trap()
    rpdb.handle_trap()

def main2():
    manager = None
    try:
        with InfiniteGlass.Display() as display:
            manager = glass_session.manager.SessionManager(display)
            InfiniteGlass.DEBUG("init", "Session manager started\n")
    except Exception as e:
        print("Session manager systemic failure, restarting: %s" % (e,))
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
    if os.environ.get("GLASS_PROFILE_glass_session", "0") == "1":
        import cProfile
        cProfile.runctx('main2()', globals(), locals(), "glass-session.prof")
    else:
        main2()

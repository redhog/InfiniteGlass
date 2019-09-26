import InfiniteGlass
import glass_ghosts.manager
import glass_ghosts.session
import sys
import traceback
        
def main2():
    try:
        with InfiniteGlass.Display() as display:
            manager = glass_ghosts.manager.GhostManager(display)
            sys.stdout.write("%s\n" % manager.session.listen_address())
            sys.stdout.flush()
            sys.stderr.write("Session manager listening to %s\n" % manager.session.listen_address())
            sys.stderr.flush()
    except Exception as e:
        print(e)
        traceback.print_exc()
    print("END")
        
def main():
    import cProfile
    cProfile.runctx('main2()', globals(), locals(), "prof.prof")

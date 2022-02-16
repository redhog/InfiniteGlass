import InfiniteGlass
import glass_ghosts.manager
import glass_ghosts.session
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
            overlay = display.root.composite_get_overlay_window().overlay_window
            overlay_geom = overlay.get_geometry()
            
            gc = overlay.create_gc(
                foreground = display.screen().black_pixel,
                background = display.screen().white_pixel)
            overlay.rectangle(gc, 0, 0, overlay_geom.width, overlay_geom.height, onerror = None)
            
            manager = glass_ghosts.manager.GhostManager(display)
            sys.stdout.write("%s\n" % manager.session.listen_address())
            sys.stdout.flush()
            InfiniteGlass.DEBUG("init", "Session manager listening to %s\n" % manager.session.listen_address())
    except Exception as e:
        print("Ghost manager systemic failure, restarting: %s" % (e,))
        traceback.print_exc()
        os.execlp(sys.argv[0], *sys.argv)        
    print("END")

def main():
    if os.environ.get("GLASS_PROFILE_glass_ghosts", "0") == "1":
        import cProfile
        cProfile.runctx('main2()', globals(), locals(), "glass-ghosts.prof")
    else:
        main2()

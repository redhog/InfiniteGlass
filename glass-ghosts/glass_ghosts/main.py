import InfiniteGlass
import glass_ghosts.manager
import glass_ghosts.session
import distutils.spawn
import sys
import traceback
import signal
import os
import time
import click

if os.environ.get("GLASS_DEBUGGER", "") == "rpdb":
    import rpdb
    rpdb.handle_trap()
    rpdb.handle_trap()

def substring_in_list(s, lst):
    for item in lst:
        if s in item:
            return True
    return False

def setup_annotator():
    preloads = []
    if "LD_PRELOAD" in os.environ:
        preloads = os.environ["LD_PRELOAD"].split(" ")
    if not substring_in_list('glass-annotator', preloads):
        preloads.append(distutils.spawn.find_executable('glass-annotator'))
        os.environ["LD_PRELOAD"] = " ".join(preloads)

@click.command()
@click.option('--restart-components/--no-restart-components', default=True)
@InfiniteGlass.profilable
def main(**kw):
    setup_annotator()
    manager = None
    try:
        with InfiniteGlass.Display() as display:
            overlay = display.root.composite_get_overlay_window().overlay_window
            overlay_geom = overlay.get_geometry()
            
            gc = overlay.create_gc(
                foreground = display.screen().black_pixel,
                background = display.screen().white_pixel)
            overlay.rectangle(gc, 0, 0, overlay_geom.width, overlay_geom.height, onerror = None)
            
            manager = glass_ghosts.manager.GhostManager(display, **kw)
            sys.stdout.write("%s\n" % manager.session.listen_address())
            sys.stdout.flush()
            InfiniteGlass.DEBUG("init", "Session manager listening to %s\n" % manager.session.listen_address())
        manager.components.shutdown()
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

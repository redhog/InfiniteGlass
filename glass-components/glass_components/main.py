import InfiniteGlass
import glass_components.components
import glass_annotator
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

@click.command()
@InfiniteGlass.profilable
def main(**kw):
    glass_annotator.setup_annotator()
    components = None
    try:
        with InfiniteGlass.Display() as display:
            overlay = display.root.composite_get_overlay_window().overlay_window
            overlay_geom = overlay.get_geometry()
            
            gc = overlay.create_gc(
                foreground = display.screen().black_pixel,
                background = display.screen().white_pixel)
            overlay.rectangle(gc, 0, 0, overlay_geom.width, overlay_geom.height, onerror = None)

            components = glass_components.components.Components(display, **kw)

        components.shutdown()
    except Exception as e:
        print("Components manager systemic failure, restarting: %s" % (e,))
        traceback.print_exc()
        try:
            if components is not None and hasattr(components, "components_by_pid"):
                for pid in components.components_by_pid.keys():
                    os.kill(pid, signal.SIGINT)
        except Exception as e:
            print(e)
            traceback.print_exc()
        os.execlp(sys.argv[0], *sys.argv)
    print("END")

import Xlib.X
import glass_ghosts.ghost
import glass_ghosts.window
import sys
import traceback

class RootWindow(object):
    def __init__(self, manager, display, **kw):
        self.manager = manager
        self.display = display

        @display.root.on(mask="SubstructureNotifyMask")
        def MapNotify(win, event):
            self.map_window(event.window)

        @display.root.on(mask="StructureNotifyMask", client_type="IG_GHOSTS_EXIT")
        def ClientMessage(win, event):
            print("message", "RECEIVED EXIT"); sys.stderr.flush()
            self.manager.shutdown()
            
        for child in display.root.query_tree().children:
            self.map_window(child)

    def map_window(self, win):
        try:
            client_win = win.find_client_window()
            if client_win is None: return

            if client_win.__window__() not in self.manager.windows:
                win = glass_ghosts.window.Window(self.manager, client_win)
                if win is not None:
                    self.manager.windows[client_win.__window__()] = win
        except Xlib.error.BadWindow as e:
            print("map_window(%s): %s" % (win, e))
            traceback.print_exc()


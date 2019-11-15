import Xlib.X
import glass_ghosts.shadow
import glass_ghosts.window

class RootWindow(object):
    def __init__(self, manager, display):
        self.manager = manager
        self.display = display

        @display.root.on(mask="SubstructureNotifyMask")
        def MapNotify(win, event):
            self.map_window(event.window)

        for child in display.root.query_tree().children:
            self.map_window(child)

    def map_window(self, win):
        try:
            if win.get_attributes().override_redirect:
                return

            client_win = win.find_client_window()
            if client_win is None: return

            if set(self.manager.IGNORE).intersection(set(client_win.keys())):
                return

            if client_win.__window__() not in self.manager.windows:
                self.manager.windows[client_win.__window__()] = glass_ghosts.window.Window(self.manager, client_win)
        except Xlib.error.BadWindow:
            pass

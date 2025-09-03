import Xlib.X
import InfiniteGlass

def island_windows(display, island):
    coords = list(island["IG_COORDS"])
    coords[1] -= coords[3]

    visible, overlap, invisible = InfiniteGlass.windows.get_windows(display, coords)

    return visible + overlap
    

def island_toggle_sleep(self, event):
    island = self.get_event_window(event)
    if island and island != self.display.root:
        old = island.get("IG_ISLAND_PAUSED", 0)
        if old == 1:
            is_paused = 0
        else:
            is_paused = 1
        InfiniteGlass.DEBUG("island_toggle_sleep", "%s.IG_ISLAND_PAUSED=%s\n" % (island, is_paused))
        island["IG_ISLAND_PAUSED"] = is_paused

        windows = [win for win, coords in island_windows(self.display, island)]

        clients_done = set()
        if is_paused:
            for win in windows:
                if "IG_GHOST" not in win:
                    if "SM_CLIENT_ID" not in win or win["SM_CLIENT_ID"] not in clients_done:
                        win.send(win, "IG_SLEEP", event_mask=Xlib.X.StructureNotifyMask)
                        if "SM_CLIENT_ID" in win:
                            clients_done.add(win["SM_CLIENT_ID"])
        else:
            for win in windows:
                if "IG_GHOST" in win and "SM_CLIENT_ID" in win and win["SM_CLIENT_ID"] not in clients_done:
                    win.send(win, "IG_RESTART", event_mask=Xlib.X.StructureNotifyMask)
                    clients_done.add(win["SM_CLIENT_ID"])
        self.display.flush()

def island_delete(self, event):
    island = self.get_event_window(event)
    if island and island != self.display.root:
        for win, coords in island_windows(self.display, island):
            win.send(win, "IG_DELETE", event_mask=Xlib.X.StructureNotifyMask)
        island.send(island, "IG_CLOSE", event_mask=Xlib.X.StructureNotifyMask)

def island_ungroup(self, event):
    island = self.get_event_window(event)
    if island and island != self.display.root:
        island.send(island, "IG_CLOSE", event_mask=Xlib.X.StructureNotifyMask)

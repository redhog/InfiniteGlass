import Xlib.X
import InfiniteGlass
import os
import datetime
from .. import mode

def keymap(self, event, value):
    self.handle(event, keymap=value)

class Formatter(object):
    def __init__(self, obj):
        self.obj = obj

    def __getitem__(self, name):
        res = self.obj[name]
        if isinstance(res, Xlib.display.drawable.Window):
            return res.__window__()
        return res
    
def shell(self, event, value):
    cmd = value % Formatter(self)
    InfiniteGlass.DEBUG("final_action", "Shell command %s\n" % (cmd,))
    os.system(cmd)

def timer(self, event, value):
    self.state[value] = datetime.datetime.now()

def counter(self, event, value):
    self.state[value] = 0

def inc(self, event, value):
    self.state[value] = self.state.get(value, 0) + 1

    
def toggle_ghosts_enabled(self, event):
    win = self.get_event_window(event)
    if win and win != self.display.root:
        old = win.get("IG_GHOSTS_DISABLED", 0)
        if old == 1:
            value = 0
        else:
            value = 1
        InfiniteGlass.DEBUG("toggle_ghosts_enabled", "%s.IG_GHOSTS_DISABLED=%s\n" % (win, value))
        win["IG_GHOSTS_DISABLED"] = value
        self.display.flush()

def toggle_overlay(self, event, show=None):
    "Slide your toolbars and widgets in/out of view"
    size = self.display.root["IG_VIEW_OVERLAY_SIZE"]
    if show is None: show = self.display.root["IG_VIEW_OVERLAY_VIEW"][0] != 0.
    height = size[1] / size[0]
    
    if show:
        self.display.root["IG_VIEW_OVERLAY_VIEW_ANIMATE"] = [0., 0., 1., height]
    else:
        self.display.root["IG_VIEW_OVERLAY_VIEW_ANIMATE"] = [.4, .4 * height, .2, .2 * height]
    self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", self.display.root, "IG_VIEW_OVERLAY_VIEW", .5)

def send_exit(self, event):
    "Ends your InfiniteGlass session"
    InfiniteGlass.DEBUG("debug", "SENDING EXIT\n")
    self.display.root.send(
        self.display.root, "IG_GHOSTS_EXIT",
        event_mask=Xlib.X.StructureNotifyMask|Xlib.X.SubstructureRedirectMask)
    self.display.flush()
    self.display.exit()
    
def send_debug(self, event):
    "Make glass-renderer print its state to stdout"
    InfiniteGlass.DEBUG("debug", "SENDING DEBUG\n")
    self.display.root.send(
        self.display.root, "IG_DEBUG",
        event_mask=Xlib.X.StructureNotifyMask|Xlib.X.SubstructureRedirectMask)
    self.display.flush()

def send_close(self, event):
    "Close the active window"
    win = self.get_event_window(event)
    InfiniteGlass.DEBUG("close", "Close %s %s\n" % (win, win.get("WM_NAME", None)))
    if win and win != self.display.root:
        InfiniteGlass.DEBUG("close", "SENDING CLOSE %s\n" % win)
        win.send(win, "IG_CLOSE", event_mask=Xlib.X.StructureNotifyMask)
        self.display.flush()

def send_sleep(self, event):
    "Make the active application store its state and exit"
    win = self.get_event_window(event)
    InfiniteGlass.DEBUG("sleep", "Sleep %s %s\n" % (win, win.get("WM_NAME", None)))
    if win and win != self.display.root:
        InfiniteGlass.DEBUG("sleep", "SENDING SLEEP %s\n" % win)
        win.send(win, "IG_SLEEP", event_mask=Xlib.X.StructureNotifyMask)
        self.display.flush()

def toggle_sleep(self, event):
    win = self.get_event_window(event)
    if win and win != self.display.root:
        if win.get("IG_GHOST", None):
            InfiniteGlass.DEBUG("restart", "SENDING RESTART %s\n" % win)
            win.send(win, "IG_RESTART", event_mask=Xlib.X.StructureNotifyMask)
            self.display.flush()
        else:
            InfiniteGlass.DEBUG("sleep", "SENDING SLEEP %s\n" % win)
            win.send(win, "IG_SLEEP", event_mask=Xlib.X.StructureNotifyMask)
            self.display.flush()    
    
def reload(self, event):
    "Reload your keybindings from the config file"
    mode.load_config()

def send_island_create(self, event):
    InfiniteGlass.DEBUG("island", "SENDING CREATE ISLAND\n")
    self.display.root.send(self.display.root, "IG_ISLAND_CREATE", event_mask=Xlib.X.StructureNotifyMask)
    self.display.flush()

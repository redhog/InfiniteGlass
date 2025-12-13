#!/usr/bin/env python3
import os
import subprocess
import sys
import InfiniteGlass
from Xlib import X, Xatom, protocol

try:
    import dbus
except ImportError:
    dbus = None

def send_client_message(display, atom, data=0):
    ev = protocol.event.ClientMessage(
        window=display.root.xid,
        client_type=atom,
        data=(32, [data, 0, 0, 0, 0])
    )
    display.root.send_event(ev, event_mask=X.SubstructureNotifyMask)
    display.flush()

def setup_unlock_detector(display, config):
    """
    Installs the locker-specific 'unlock detected' callback.
    When triggered, the callback sends WM_SCREEN_UNLOCKED.
    """

    atom_unlocked = display.intern_atom("WM_SCREEN_UNLOCKED")

    locker = config["locker"]["cmd"].split()
    locker_type = config["locker"].get("type", locker[0])
    
    if locker_type in ("i3lock", "slock"):
        proc = subprocess.Popen(locker)

        @display.on_child_exit(pid=proc.pid)
        def child_exit(ev):
            send_client_message(display, atom_unlocked)

        return
    
    elif locker_type == "xscreensaver":
        proc = subprocess.Popen(
            ["xscreensaver-command", "-watch"],
            stdout=subprocess.PIPE,
            text=True
        )

        @display.mainloop.add_lines(proc.stdout.fileno())
        def xscreensaver_watch(fd, line):
            line = line.strip()
            if line == "UNBLANK": # can be UNBLANK, BLANK, LOCK
                send_client_message(display, atom_unlocked)

        subprocess.run(["xscreensaver-command", "-lock"])
        return

    elif locker_type in ("gnome", "xfce", "light") and dbus:
        session = dbus.SessionBus()

        if locker_type == "gnome":
            obj = session.get_object("org.gnome.ScreenSaver", "/org/gnome/ScreenSaver")
        elif locker_type == "xfce":
            obj = session.get_object("org.xfce.ScreenSaver", "/org/xfce/ScreenSaver")
        elif locker_type == "light":
            obj = session.get_object("org.freedesktop.ScreenSaver", "/org/freedesktop/ScreenSaver")

        iface = dbus.Interface(obj, "org.freedesktop.ScreenSaver")

        def active_changed(active):
            if not active:
                send_client_message(display, atom_unlocked)

        session.add_signal_receiver(
            active_changed,
            signal_name="ActiveChanged",
            dbus_interface="org.freedesktop.ScreenSaver"
        )

        iface.Lock()
        return

    else:
        print(f"Unknown or unsupported locker type: {locker_type}")
        return

@InfiniteGlass.profilable
def main(*arg, **kw):
    with InfiniteGlass.Display() as display:
        configpath = os.path.expanduser(os.environ.get("GLASS_WIDGET_CONFIG", "~/.config/glass/lock.yml"))
        with open(configpath) as f:
            config = InfiniteGlass.load_yaml(f, display)

        atom_lock     = display.intern_atom("WM_SCREEN_LOCKED")
        atom_unlocked = display.intern_atom("WM_SCREEN_UNLOCKED")

        @display.root.on()
        def ClientMessage(win, ev):
            if ev.client_type == atom_lock:
                setup_unlock_detector(display, config)

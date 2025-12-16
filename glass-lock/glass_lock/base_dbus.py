from . import base
import time

try:
    import dbus
except ImportError:
    dbus = None

class LockerDBUS(base.Locker):
    cmd = ["gnome-screensaver"]
    bus_name = "org.gnome.ScreenSaver"
    obj_path = "/org/gnome/ScreenSaver"

    def __init__(self, *arg, **kw):
        base.Locker.__init__(self, *arg, **kw)
        self.session = dbus.SessionBus()        
        self.ensure_dbus_locker_running()
    
    def ensure_dbus_locker_running(self):
        dbus_iface = dbus.Interface(
            self.session.get_object(
                "org.freedesktop.DBus",
                "/org/freedesktop/DBus"),
            "org.freedesktop.DBus")

        if not dbus_iface.NameHasOwner(self.bus_name):
            subprocess.Popen(self.cmd)
            for _ in range(20):
                if dbus_iface.NameHasOwner(self.bus_name):
                    return
                time.sleep(0.1)
            raise Exception("Unable to start locker %s" % self)

    def active_changed(self, active):
        if not active:
            self.unlocked()

    def lock(self):
        iface = dbus.Interface(
            self.session.get_object(self.bus_name, self.obj_path),
            "org.freedesktop.ScreenSaver")
        session.add_signal_receiver(
            self.active_changed,
            signal_name="ActiveChanged",
            dbus_interface="org.freedesktop.ScreenSaver"
        )
        iface.Lock()

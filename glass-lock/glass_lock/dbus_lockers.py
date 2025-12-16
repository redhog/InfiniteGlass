from . import base_dbus

class LockerGnome(base_dbus.LockerDBUS):
    cmd = ["gnome-screensaver"]
    bus_name = "org.gnome.ScreenSaver"
    obj_path = "/org/gnome/ScreenSaver"
    
class LockerMate(base_dbus.LockerDBUS):
    cmd = ["mate-screensaver"]
    bus_name = "org.gnome.ScreenSaver"
    obj_path = "/org/gnome/ScreenSaver"
    
class LockerXfce4(base_dbus.LockerDBUS):
    cmd = ["xfce4-screensaver"]
    bus_name = "org.xfce.ScreenSaver"
    obj_path = "/org/xfce/ScreenSaver"
    
class LockerLight(base_dbus.LockerDBUS):
    cmd = ["gnome-screensaver"]
    bus_name = "org.freedesktop.ScreenSaver"
    obj_path = "/org/freedesktop/ScreenSaver"

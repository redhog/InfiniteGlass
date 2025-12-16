#!/usr/bin/env python3
import os
import subprocess
import sys
import importlib.metadata
import InfiniteGlass
import Xlib

def screen_lock(self, **kw):
    print("Sending screen lock message")
    self.display.root.send(
        self.display.root, "IG_SCREEN_LOCK", 0,
        event_mask=Xlib.X.StructureNotifyMask)

@InfiniteGlass.profilable
def main(*arg, **kw):
    
    with InfiniteGlass.Display() as display:
        configpath = os.path.expanduser(
            os.environ.get("GLASS_WIDGET_CONFIG", "~/.config/glass/lock.yml")
        )

        with open(configpath) as f:
            config = InfiniteGlass.load_yaml(f, display)

        locker_name, locker_args = next(iter(config["locker"].items()))
        locker_args = locker_args or {}
        LockerCls = importlib.metadata.entry_points(
            group="InfiniteGlass.lockers")[locker_name].load()

        LockerCls(
            display, config, **locker_args)


if __name__ == "__main__":
    main()

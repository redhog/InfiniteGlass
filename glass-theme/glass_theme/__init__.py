import InfiniteGlass
import pkg_resources
import json
import numpy
import Xlib.X
import sys
import re
import os.path
import yaml
import importlib

def main(*arg, **kw):
    with InfiniteGlass.Display() as display:
        configpath = os.path.expanduser(os.environ.get("GLASS_THEME_CONFIG", "~/.config/glass/theme.json"))
        with open(configpath) as f:
            config = yaml.load(f, Loader=yaml.SafeLoader)

        module_name, cls_name = config["name"].rsplit(".", 1)
        module = importlib.import_module(module_name)
        importlib.reload(module)
        cls = getattr(module, cls_name)
        
        cls(display, **config.get("args", {}))

        InfiniteGlass.DEBUG("init", "Theme started: %s\n" % config["name"])

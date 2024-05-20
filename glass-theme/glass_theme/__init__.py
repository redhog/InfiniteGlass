import InfiniteGlass
import pkg_resources
import json
import numpy
import Xlib.X
import sys
import re
import os.path
import yaml
from .utils import instantiate_config

@InfiniteGlass.profilable
def main(*arg, **kw):
    with InfiniteGlass.Display() as display:
        if sys.argv[1:]:
            config = json.loads(sys.argv[1])
        else:
            configpath = os.path.expanduser(os.environ.get("GLASS_THEME_CONFIG", "~/.config/glass/theme.yml"))
            with open(configpath) as f:
                config = yaml.load(f, Loader=yaml.SafeLoader)

        theme = instantiate_config(display, None, config)
        theme.activate()
        display.flush()
        
        InfiniteGlass.DEBUG("init", "Theme started: %s\n" % theme)

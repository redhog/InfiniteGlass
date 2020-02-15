import InfiniteGlass
import pkg_resources
import json
import numpy
import Xlib.X
import sys
import re
import glass_theme.default

def main(*arg, **kw):
    with InfiniteGlass.Display() as display:        
        glass_theme.default.Theme(display)
        InfiniteGlass.DEBUG("init", "Theme started\n")

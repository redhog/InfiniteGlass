import InfiniteGlass
import Xlib.X
import Xlib.Xcursorfont
import Xlib.keysymdef.miscellany
import Xlib.ext.xinput
import os.path
import pkg_resources
import json
import click
import sys
import sqlite3
import yaml

import glass_action.main
import glass_action.window_tools


@glass_action.main.main.command()
@click.argument("value")
@click.pass_context
def expression(ctx, value):
    with InfiniteGlass.Display() as display:
        try:
            value = json.loads(value, object_hook=InfiniteGlass.fromjson(display))
        except:
            pass
        itemtype, items, fmt = InfiniteGlass.parse_value(display, value)
        print("%s(%s): %s" % (itemtype, fmt, items))
        sys.exit(0)


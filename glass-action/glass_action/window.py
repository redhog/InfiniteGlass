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


@glass_action.main.main.group()
@click.pass_context
def window(ctx, **kw):
    pass

@window.command()
@click.option('--window', default="click")
@click.option('--mask', default="StructureNotifyMask")
@click.argument("event", nargs=-1)
@click.pass_context
def send(ctx, window, mask, event):
    with InfiniteGlass.Display() as display:
        if window == "click":
            @glass_action.window_tools.get_pointer_window(display)
            def with_win(window):
                glass_action.window_tools.send_msg(display, window, mask, event)
                sys.exit(0)
        else:
            glass_action.window_tools.send_msg(display, window, mask, event)
            sys.exit(0)
            
@window.group()
@click.pass_context
def inspect(ctx, **kw):
    pass

@inspect.command()
@click.option('--window', default="click")
@click.pass_context
def key(ctx, window):
    with InfiniteGlass.Display() as display:
        def inspect(win):
            if isinstance(win, str):
                if win == "root":
                    win = display.root
                else:
                    win = display.create_resource_object("window", int(win))
            import glass_ghosts.helpers

            configpath = os.path.expanduser(os.environ.get("GLASS_GHOSTS_CONFIG", "~/.config/glass/ghosts.json"))
            with open(configpath) as f:
                config = yaml.load(f, Loader=yaml.SafeLoader)

            print(glass_ghosts.helpers.ghost_key(win, config["match"]))
        if window == "click":
            @glass_action.window_tools.get_pointer_window(display)
            def with_win(window):
                inspect(window)
                sys.exit(0)
        else:
            inspect(window)
            sys.exit(0)

@inspect.command()
@click.option('--window', default="click")
@click.option('--limit', default=0)
@click.pass_context
def props(ctx, window, limit):
    with InfiniteGlass.Display() as display:
        def inspect(win):
            if isinstance(win, str):
                if win == "root":
                    win = display.root
                else:
                    win = display.create_resource_object("window", int(win))
            for key, value in win.items():
                value = str(value)
                if limit > 0:
                    value = value[:limit]
                print("%s=%s" % (key, value))
        if window == "click":
            @glass_action.window_tools.get_pointer_window(display)
            def with_win(window):
                inspect(window)
                sys.exit(0)
        else:
            inspect(window)
            sys.exit(0)
            

@window.command()
@click.argument("name")
@click.argument("animation")
@click.pass_context
def animate(ctx, name, animation):
    with InfiniteGlass.Display() as display:
        anim = display.root["IG_ANIMATE"]
        name += "_SEQUENCE"
        display.root[name] = json.loads(animation)
        anim.send(anim, "IG_ANIMATE", display.root, name, 0.0, event_mask=Xlib.X.PropertyChangeMask)
        display.flush()
        sys.exit(0)


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
        @glass_action.window_tools.send_msg(display, window, mask, event)
        def done():
            sys.exit(0)
            

@window.command()
@click.option('--window', default="click")
@click.argument('name')
@click.argument('value')
@click.pass_context
def set(ctx, window, name, value):
    with InfiniteGlass.Display() as display:
        @glass_action.window_tools.str_to_win(display, window)
        def set(win):
            v = value
            try:
                v = json.loads(v)
            except:
                pass
            properties = json.loads(
                json.dumps({name: v}),
                object_hook=InfiniteGlass.fromjson(display))
            for k, v in properties.items():
                win[k] = v
            display.flush()
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
        @glass_action.window_tools.str_to_win(display, window)
        def inspect(win):
            import glass_ghosts.helpers

            configpath = os.path.expanduser(os.environ.get("GLASS_GHOSTS_CONFIG", "~/.config/glass/ghosts.json"))
            with open(configpath) as f:
                config = yaml.load(f, Loader=yaml.SafeLoader)

            print(glass_ghosts.helpers.ghost_key(win, config["match"]))
            sys.exit(0)

@inspect.command()
@click.option('--window', default="click")
@click.option('--limit', default=0)
@click.pass_context
def props(ctx, window, limit):
    with InfiniteGlass.Display() as display:
        @glass_action.window_tools.str_to_win(display, window)
        def inspect(win):
            for key, value in win.items():
                value = str(value)
                if limit > 0:
                    value = value[:limit]
                print("%s=%s" % (key, value))
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


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
@click.pass_context
def list(ctx):
    with InfiniteGlass.Display() as display:
        for window in display.root.query_tree().children:
            if window.get_attributes().map_state != Xlib.X.IsViewable: continue
            child = window.find_client_window()
            if child is None: continue

            coords = child.get("IG_COORDS", None)
            size = child.get("IG_SIZE", None)
            name = child.get("WM_NAME", None)
            cls = child.get("WM_CLASS", None)
            layer = child.get("IG_LAYER", "IG_LAYER_DESKTOP")

            name = name and name.decode("utf-8"),
            cls = cls and [c.decode("utf-8") for c in cls]

            print("%s:%s%s%s in %s" % (
                child,
                (" (%s)" % ",".join(cls)) if cls else "",
                (" @ %f,%f,%f,%f" % coords if coords else ""),
                (" [%d,%d]" % size if size else ""),
                layer))

@window.command()
@click.option('--window', default="click")
@click.option('--mask', default="StructureNotifyMask")
@click.argument("event", nargs=-1)
@click.pass_context
def send(ctx, window, mask, event):
    """Usage examples:

\b
    glass-action window send --mask SubstructureNotifyMask --window root IG_DEBUG_PICKING
    glass-action window send --mask StructureNotifyMask --window root IG_GHOSTS_EXIT
    glass-action window send --mask SubstructureNotifyMask --window root IG_EXIT
    """
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
    """Name is the property atom name as a string.

Value is the property value as a JSON value, parsed using
InfiniteGlass.fromjson (so any __jsonclass__ values supported by it
are supported). In particular, strings are interpreted as atom names,
actual string values need to use the __jsonclass__ encoding.

Usage examples:

\b
    glass-action window set --window 1234567 IG_COORDS "[1.0, 1.0, 1.0, 1.0]"

    """
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

            configpath = os.path.expanduser(os.environ.get("GLASS_GHOSTS_CONFIG", "~/.config/glass/ghosts.yml"))
            with open(configpath) as f:
                config = yaml.load(f, Loader=yaml.SafeLoader)

            print(glass_ghosts.helpers.ghost_key(win, config["match"]))
            sys.exit(0)

@inspect.command()
@click.option('--window', default="click")
@click.option('--name')
@click.option('--limit', default=0)
@click.pass_context
def props(ctx, window, name, limit):
    with InfiniteGlass.Display() as display:
        @glass_action.window_tools.str_to_win(display, window)
        def inspect(win):
            try:
                if name is not None:
                    items = [(name, win[name])]
                else:
                    items = win.items()
                for key, value in items:
                    value = str(value)
                    if limit > 0:
                        value = value[:limit]
                    print("%s=%s" % (key, value))
            except Exception as e:
                print(e)
            finally:
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


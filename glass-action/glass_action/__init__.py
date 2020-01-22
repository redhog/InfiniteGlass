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

@click.group()
@click.pass_context
def main(ctx, **kw):
    ctx.obj = {}
    
def send_msg(display, win, mask, event):
    if isinstance(win, str):
        if win == "root":
            win = display.root
        else:
            win = display.create_resource_object("window", int(win))
    def conv(item):
        try:
            if "." in item:
                return float(item)
            else:
                return int(item)
        except:
            return item
    event = [conv(item) for item in event]
    print("SEND win=%s, mask=%s %s" % (win, mask, ", ".join(repr(item) for item in event)))
    win.send(win, *event, event_mask=getattr(Xlib.X, mask))
    display.flush()


def get_pointer_window(display):
    def get_pointer_window(cb):
        font = display.open_font('cursor')
        input_cursor = font.create_glyph_cursor(
            font, Xlib.Xcursorfont.box_spiral, Xlib.Xcursorfont.box_spiral + 1,
            (65535, 65535, 65535), (0, 0, 0))
        
        display.root.grab_pointer(
            False, Xlib.X.ButtonPressMask | Xlib.X.ButtonReleaseMask,
            Xlib.X.GrabModeAsync, Xlib.X.GrabModeAsync, display.root, input_cursor, Xlib.X.CurrentTime)

        @display.on()
        def ButtonPress(win, event):
            display.ungrab_pointer(Xlib.X.CurrentTime)
            window = event.child
            window = window.find_client_window()
            cb(window)
    return get_pointer_window

@main.command()
@click.option('--window', default="click")
@click.option('--mask', default="StructureNotifyMask")
@click.argument("event", nargs=-1)
@click.pass_context
def send(ctx, window, mask, event):
    with InfiniteGlass.Display() as display:
        if window == "click":
            @get_pointer_window(display)
            def with_win(window):
                send_msg(display, window, mask, event)
                sys.exit(0)
        else:
            send_msg(display, window, mask, event)
            sys.exit(0)


@main.command()
@click.option('--window', default="click")
@click.option('--limit', default=0)
@click.pass_context
def inspect(ctx, window, limit):
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
            @get_pointer_window(display)
            def with_win(window):
                inspect(window)
                sys.exit(0)
        else:
            inspect(window)
            sys.exit(0)

            
@main.command()
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
            
@main.group()
@click.pass_context
def shadow(ctx, **kw):
    pass

@shadow.command()
@click.pass_context
def list(ctx):
    dbpath = os.path.expanduser("~/.config/glass/ghosts.sqlite3")
    dbconn = sqlite3.connect(dbpath)
    cur = dbconn.cursor()
    cur.execute("select key from shadows group by key order by key")
    for (key,) in cur:
        print(key)

@shadow.command()
@click.argument("key", nargs=-1)
@click.pass_context
def export(ctx, key):
    dbpath = os.path.expanduser("~/.config/glass/ghosts.sqlite3")
    dbconn = sqlite3.connect(dbpath)
    cur = dbconn.cursor()
    res = {}
    for k in key:
        cur.execute('select name, value from shadows where key = ?', (k,))
        properties = {}
        currentkey = None
        for name, value in cur:
            properties[name] = json.loads(value)
        res[k] = properties
    print(json.dumps(res))

@shadow.command(name="import")
@click.pass_context
def imp(ctx):
    shadows = json.load(sys.stdin)
    dbpath = os.path.expanduser("~/.config/glass/ghosts.sqlite3")
    dbconn = sqlite3.connect(dbpath)
    cur = dbconn.cursor()
    for key, shadow in shadows.items():
        for name, value in shadow.items():
            cur.execute("insert into shadows (key, name, value) values (?, ?, ?)",
                        (key, name, json.dumps(value)))
    dbconn.commit()


@shadow.command()
@click.argument("key")
@click.pass_context
def delete(ctx, key):
    dbpath = os.path.expanduser("~/.config/glass/ghosts.sqlite3")
    dbconn = sqlite3.connect(dbpath)
    cur = dbconn.cursor()
    cur.execute('delete from shadows where key = ?', (key,))
    dbconn.commit()


@main.group()
@click.pass_context
def component(ctx, **kw):
    pass

@component.command()
@click.pass_context
def list(ctx):
    with InfiniteGlass.Display() as display:
        for key in display.root.keys():
            if key.startswith("IG_COMPONENT_"):
                print(key[len("IG_COMPONENT_"):])
    
@component.command()
@click.argument("name")
@click.argument("command", nargs=-1)
@click.pass_context
def start(ctx, name, command):
    with InfiniteGlass.Display() as display:
        key = "IG_COMPONENT_%s" % name
        display.root[key] = json.dumps({"command": command, "name": name}).encode("utf-8")
        display.flush()


@component.command()
@click.argument("name")
@click.pass_context
def show(ctx, name):
    with InfiniteGlass.Display() as display:
        key = "IG_COMPONENT_%s" % name
        print(" ".join(json.loads(display.root[key].decode("utf-8"))["command"]))
        
@component.command()
@click.argument("name")
@click.pass_context
def restart(ctx, name):
    with InfiniteGlass.Display() as display:
        key = "IG_COMPONENT_%s" % name
        display.root[key] = display.root[key]
        display.flush()

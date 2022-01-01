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
def component(ctx, **kw):
    pass

@component.command()
@click.pass_context
def list(ctx):
    with InfiniteGlass.Display() as display:
        for key in display.root.keys():
            if key.startswith("IG_COMPONENT_"):
                name = key[len("IG_COMPONENT_"):]
                pidkey = "IG_COMPONENTPID_" + name
                pid = "not running"
                if pidkey in display.root:
                    pid = str(display.root[pidkey])
                print("%s (%s)" % (name, pid))
    
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

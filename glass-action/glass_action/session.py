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
def session(ctx, **kw):
    pass

@session.command()
@click.option('--long', is_flag=True)
@click.pass_context
def list(ctx, long):
    dbpath = os.path.expanduser("~/.config/glass/ghosts.sqlite3")
    dbconn = sqlite3.connect(dbpath)
    cur = dbconn.cursor()
    cur.execute("""
      select key, (select c2.value from clients as c2 where c2.key = c1.key and c2.name = "RestartCommand") as start
      from clients as c1 group by key order by key
    """)
    for (key, start) in cur:
        key = key.decode("utf-8")
        if long:
            start = " ".join(json.loads(start)[1])
            print("%s %s" % (key, start))
        else:
            print(key)

@session.command()
@click.argument("key", nargs=-1)
@click.pass_context
def export(ctx, key):
    dbpath = os.path.expanduser("~/.config/glass/ghosts.sqlite3")
    dbconn = sqlite3.connect(dbpath)
    cur = dbconn.cursor()
    res = {}
    if not key:
        cur.execute("""
          select key from clients as c1 group by key order by key
        """)
        key = [row[0].decode("utf-8") for row in cur]
    for k in key:
        cur.execute('select name, value from clients where key = ?', (k.encode("utf-8"),))
        properties = {}
        currentkey = None
        for name, value in cur:
            properties[name] = json.loads(value)
        res[k] = properties
    print(json.dumps(res))

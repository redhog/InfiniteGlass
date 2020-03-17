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
def ghost(ctx, **kw):
    pass

@ghost.command()
@click.pass_context
def list(ctx):
    dbpath = os.path.expanduser("~/.config/glass/ghosts.sqlite3")
    dbconn = sqlite3.connect(dbpath)
    cur = dbconn.cursor()
    cur.execute("select key from ghosts group by key order by key")
    for (key,) in cur:
        print(key)

@ghost.command()
@click.argument("key", nargs=-1)
@click.pass_context
def export(ctx, key):
    dbpath = os.path.expanduser("~/.config/glass/ghosts.sqlite3")
    dbconn = sqlite3.connect(dbpath)
    cur = dbconn.cursor()
    res = {}
    if not key:
        cur.execute("select key from ghosts group by key order by key")
        key = [row[0] for row in cur]
    for k in key:
        cur.execute('select name, value from ghosts where key = ?', (k,))
        properties = {}
        currentkey = None
        for name, value in cur:
            properties[name] = json.loads(value)
        res[k] = properties
    print(json.dumps(res))

@ghost.command(name="import")
@click.pass_context
def imp(ctx):
    ghosts = json.load(sys.stdin)
    dbpath = os.path.expanduser("~/.config/glass/ghosts.sqlite3")
    dbconn = sqlite3.connect(dbpath)
    cur = dbconn.cursor()
    for key, ghost in ghosts.items():
        for name, value in ghost.items():
            cur.execute("insert into ghosts (key, name, value) values (?, ?, ?)",
                        (key, name, json.dumps(value)))
    dbconn.commit()


@ghost.command()
@click.argument("key")
@click.pass_context
def delete(ctx, key):
    dbpath = os.path.expanduser("~/.config/glass/ghosts.sqlite3")
    dbconn = sqlite3.connect(dbpath)
    cur = dbconn.cursor()
    cur.execute('delete from ghosts where key = ?', (key,))
    dbconn.commit()



@ghost.command()
@click.argument("key")
@click.pass_context
def session(ctx, key):
    dbpath = os.path.expanduser("~/.config/glass/ghosts.sqlite3")
    dbconn = sqlite3.connect(dbpath)
    cur = dbconn.cursor()
    cur.execute('select value from ghosts where key = ? and name = "SM_CLIENT_ID"', (key,))
    client_id = json.loads(next(cur)[0], object_hook=InfiniteGlass.fromjson(None)).decode("utf-8")
    print(client_id)

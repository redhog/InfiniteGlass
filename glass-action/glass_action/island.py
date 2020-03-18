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
def island(ctx, **kw):
    pass

@island.command(name="set-background")
@click.option('--window', default="click")
@click.argument("background", nargs=-1)
@click.pass_context
def set_background(ctx, window, background):
    with InfiniteGlass.Display() as display:
        @glass_action.window_tools.str_to_win(display, window)
        def set_background(window):
            if background:
                background_path = background[0] 
            else:
                import gi
                gi.require_version('Gtk', '3.0')
                from gi.repository import Gtk as gtk
                dlg = gtk.FileChooserDialog(
                    title="Change background (SVG files only)",
                    action=gtk.FileChooserAction.OPEN,
                    buttons=(gtk.STOCK_CANCEL, gtk.ResponseType.CANCEL, gtk.STOCK_OPEN, gtk.ResponseType.OK))
                filter = gtk.FileFilter()
                filter.add_pattern("*.svg")
                dlg.set_filter(filter)
                response = dlg.run()
                background_path = dlg.get_filename()                
                dlg.destroy()
                if response != gtk.ResponseType.OK:
                    sys.exit(0)
                    return
            window["IG_CONTENT"] = ("IG_SVG", "file://" + background_path)
            display.flush()
            sys.exit(0)

import InfiniteGlass
import Xlib.X
import Xlib.Xcursorfont
import Xlib.keysymdef.miscellany
import Xlib.ext.xinput
import os.path
import pkg_resources
import json
from . import mode

def main(*arg, **kw):
    configpath = os.environ.get("GLASS_INPUT_CONFIG", "~/.config/glass/input.json")
    if configpath:
        configpath = os.path.expanduser(configpath)

        configdirpath = os.path.dirname(configpath)
        if not os.path.exists(configdirpath):
            os.makedirs(configdirpath)

        if not os.path.exists(configpath):
            with pkg_resources.resource_stream("glass_input", "config.json") as inf:
                with open(configpath, "wb") as outf:
                    outf.write(inf.read())

        with open(configpath) as f:
            mode.config = json.load(f)
    else:
        with pkg_resources.resource_stream("glass_input", "config.json") as f:
            mode.config = json.load(f)

    with InfiniteGlass.Display() as display:

        # extension_info = display.query_extension('XInputExtension')
        # xinput_major = extension_info.major_opcode
        version_info = display.xinput_query_version()
        print('Found XInput version %u.%u' % (
            version_info.major_version,
            version_info.minor_version,
        ))

        font = display.open_font('cursor')
        display.input_cursor = font.create_glyph_cursor(
            font, Xlib.Xcursorfont.box_spiral, Xlib.Xcursorfont.box_spiral + 1,
            (65535, 65535, 65535), (0, 0, 0))

        display.animate_window = -1
        @display.root.require("IG_ANIMATE")
        def animate_window(root, win):
            display.animate_window = win

        @display.eventhandlers.append
        def handle(event):
            if display.animate_window == -1:
                return False
            return mode.handle_event(display, event)

        mode.push_by_name(display, "base")

        display.root.xinput_select_events([
            (Xlib.ext.xinput.AllDevices, Xlib.ext.xinput.RawMotionMask),
        ])

        InfiniteGlass.DEBUG("init", "Input handler started\n")

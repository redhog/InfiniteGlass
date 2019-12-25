import InfiniteGlass
import Xlib.X
import Xlib.Xcursorfont
import Xlib.keysymdef.miscellany
import Xlib.ext.xinput
import os.path
import pkg_resources
import yaml
from . import mode

def main(*arg, **kw):
    mode.load_config()

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

        mode.push_by_name(display, "base_mode")

        display.root.xinput_select_events([
            (Xlib.ext.xinput.AllDevices, Xlib.ext.xinput.RawMotionMask),
        ])

        InfiniteGlass.DEBUG("init", "Input handler started\n")

import Xlib.display
import Xlib.X
import Xlib.ext.ge
import Xlib.xobject.drawable
import Xlib.protocol.event
from . import display
from . import window
from . import eventmask
from . import coords
from . import windows
from . import action
from .keymap import *
from .valueencoding import *
from .debug import *
from .profile import *
from .utils import *
from .event import EventPattern
from Xlib.display import Display

def client_message_parse(self, *types):
    disp = self.window.display.real_display
    format, data = self.data
    return [unpack_value(disp, t, value)
            for t, value in zip(types, data)]
Xlib.protocol.event.ClientMessage.parse = client_message_parse

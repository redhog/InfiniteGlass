import InfiniteGlass
import Xlib.X
import Xlib.Xcursorfont
import Xlib.keysymdef.miscellany
import Xlib.ext.xinput
import numpy
import os.path
import sys
import pkg_resources
import json
import math

config = {}

def push(display, Mode, **kw):
    InfiniteGlass.DEBUG("modes.push", "PUSH %s\n" % Mode)
    if not hasattr(display, "input_stack"):
        display.input_stack = []
    mode = Mode(display=display, **kw)
    display.input_stack.append(mode)
    if not mode.enter():
        pop(display)
        return False
    return True

def push_by_name(display, name, **kw):
    cls = getattr(sys.modules[push_by_name.__module__], config[name]["class"])
    return push(display, cls, keymap=config[name]["keymap"], **kw)
    
def pop(display):
    res = display.input_stack.pop()
    InfiniteGlass.DEBUG("modes.pop", "POP %s\n" % res)
    res.exit()
    return res

def handle_event(display, event):
    InfiniteGlass.DEBUG("event", "HANDLE %s\n" % event)
    for i in range(len(display.input_stack)-1, -1, -1):
        mode = display.input_stack[i]
        if mode.handle(event):
            InfiniteGlass.DEBUG("event", "        BY %s %s\n" % (i, mode))
            return True
    InfiniteGlass.DEBUG("event", "        UNHANDLED\n")
    return False

def view_to_space(screen, size, screenx, screeny):
    screeny = screeny - size[1] # FIXME: Merge into matrix...
    screen2space = numpy.array(((screen[2]/size[0],0,0,screen[0]),
                                (0,-screen[3]/size[1],0,screen[1]),
                                (0,0,1,0),
                                (0,0,0,1)))
    space = numpy.array((screenx, screeny, 0., 1.))
    out = screen2space.dot(space)
    return out[:2]

class Mode(object):
    def __init__(self, **kw):
        for key, value in kw.items():
            setattr(self, key, value)

    def enter(self):
        return True
    
    def exit(self):
        pass
    
    def handle(self, event):
        for key, value in self.keymap.items():
            if event[key.split(",")]:
                self.action(key, value, event)
                return True
        return True

    def action(self, key, name, event):
        if hasattr(self, name):
            getattr(self, name)(event)
        elif name in config:
            if "class" in config[name]:
                if push_by_name(self.display, name, first_event=event, last_event=event):
                    handle_event(self.display, event)
            elif "shell" in config[name]:
                os.system(config[name]["shell"])
            else:
                InfiniteGlass.DEBUG("error", "Unknown action parameters for %s" % (name,))
        else:
            InfiniteGlass.DEBUG("error", "Unknown action for %s: %\n" % (key, name)) 
    
    def pop(self, event):
        pop(self.display)
    
    def get_active_window(self):
        pointer = self.display.root.query_pointer()

        try:
            views = {self.display.root[name + "_LAYER"]: (self.display.root[name + "_VIEW"],
                                                          self.display.root[name + "_SIZE"])
                     for name in self.display.root["IG_VIEWS"]}
        except KeyError:
            return None
        spacecoords = {layer: view_to_space(view, size, pointer.root_x, pointer.root_y)
                       for layer, (view, size) in views.items()}
        for child in self.display.root.query_tree().children:
            coords = child.get("IG_COORDS", None)
            if coords is None: continue
            layer = child.get("IG_LAYER", "IG_LAYER_DESKTOP")
            pointer = spacecoords[layer]
            if (pointer[0] >= coords[0]
                and pointer[0] <= coords[0] + coords[2]
                and pointer[1] <= coords[1]
                and pointer[1] >= coords[1] - coords[3]):
                return child
        return None

    def get_event_window(self, event):
        if event == "ButtonPress":
            return self.get_active_window()
        else:
            focus = self.display.get_input_focus().focus
            if focus == Xlib.X.PointerRoot:
                return None
            return focus
        
class BaseMode(Mode):
    def __init__(self, **kw):
        Mode.__init__(self, **kw)
        for mod in range(0, 2**8):
            self.display.root.grab_key(self.display.keycode("XK_Super_L"),
                                       mod, False, Xlib.X.GrabModeAsync, Xlib.X.GrabModeAsync)
            self.display.root.grab_button(
                Xlib.X.AnyButton, Xlib.X.Mod4Mask | mod, False,
                Xlib.X.ButtonPressMask | Xlib.X.ButtonReleaseMask | Xlib.X.PointerMotionMask,
                Xlib.X.GrabModeAsync, Xlib.X.GrabModeAsync, self.display.root, self.display.input_cursor)           
        self.display.root["IG_VIEW_OVERLAY_VIEW"] = [.2, .2, .6, 0.0]

    def focus_follows_mouse(self, event):
        win = self.get_active_window()
        if win:
            win.set_input_focus(Xlib.X.RevertToNone, Xlib.X.CurrentTime)

class GrabbedMode(Mode):
    def __init__(self, **kw):
        Mode.__init__(self, **kw)
        self.display.root.grab_pointer(
            False, Xlib.X.ButtonPressMask | Xlib.X.ButtonReleaseMask | Xlib.X.PointerMotionMask,
            Xlib.X.GrabModeAsync, Xlib.X.GrabModeAsync, self.display.root, self.display.input_cursor, Xlib.X.CurrentTime)
        self.display.root.grab_keyboard(False, Xlib.X.GrabModeAsync, Xlib.X.GrabModeAsync, Xlib.X.CurrentTime)

    def exit(self):
        self.display.ungrab_pointer(Xlib.X.CurrentTime)
        self.display.ungrab_keyboard(Xlib.X.CurrentTime)

    def debugpos(self, event):
        pass
        #self.get_active_window()
        
    def toggle_overlay(self, event):
        old = self.display.root["IG_VIEW_OVERLAY_VIEW"]
        if old[0] == 0.:
            self.display.root["IG_VIEW_OVERLAY_VIEW_ANIMATE"] = [.2, .2, .6, .6 * old[3] / old[2]]
        else:
            self.display.root["IG_VIEW_OVERLAY_VIEW_ANIMATE"] = [0., 0., 1., old[3] / old[2]]
        self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", self.display.root, "IG_VIEW_OVERLAY_VIEW", .5)

    def send_debug(self, event):
        InfiniteGlass.DEBUG("debug", "SENDING DEBUG\n")
        self.display.root.send(
            self.display.root, "IG_DEBUG",
            event_mask=Xlib.X.StructureNotifyMask|Xlib.X.SubstructureRedirectMask)
    def send_close(self, event):
        win = self.get_active_window()
        if win and win != self.display.root:
            InfiniteGlass.DEBUG("close", "SENDING CLOSE %s\n" % win)
            win.send(win, "IG_CLOSE", event_mask=Xlib.X.StructureNotifyMask)

    def send_sleep(self, event):
        win = self.get_active_window()
        if win and win != self.display.root:
            InfiniteGlass.DEBUG("sleep", "SENDING SLEEP %s\n" % win)
            win.send(win, "IG_SLEEP", event_mask=Xlib.X.StructureNotifyMask)

    def zoom_1_1_1(self, event):
        win = self.get_active_window()
        if win and win != self.display.root:
            # zoom_screen_to_window_and_window_to_screen

            InfiniteGlass.DEBUG("zoom", "zoom_screen_to_window_and_window_to_screen\n")
            size = self.display.root["IG_VIEW_DESKTOP_SIZE"]
            coords = list(win["IG_COORDS"])
            screen = list(self.display.root["IG_VIEW_DESKTOP_VIEW"])

            coords[3] = (size[1] * coords[2]) / size[0]

            screen[2] = coords[2]
            screen[3] = coords[3]
            screen[0] = coords[0]
            screen[1] = coords[1] - screen[3]

            InfiniteGlass.DEBUG("zoom", "    screen=%s geom=%s\n" % (screen, size))

            win["IG_COORDS_ANIMATE"] = coords
            win["IG_SIZE_ANIMATE"] = size
            self.display.root["IG_VIEW_DESKTOP_VIEW_ANIMATE"] = screen
            self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", win, "IG_COORDS", .5)
            self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", win, "IG_SIZE", .5)
            self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", self.display.root, "IG_VIEW_DESKTOP_VIEW", .5)
            self.display.flush()
            self.display.sync()

    def zoom_home(self, event):
       InfiniteGlass.DEBUG("zoom", "ZOOM HOME\n")
       old = self.display.root["IG_VIEW_DESKTOP_VIEW"]
       self.display.root["IG_VIEW_DESKTOP_VIEW_ANIMATE"] = [0., 0., 1., old[3] / old[2]]
       self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", self.display.root, "IG_VIEW_DESKTOP_VIEW", .5)
    
class ZoomMode(Mode):
    def zoom(self, factor, around_aspect = (0.5, 0.5), around_pos = None, view="IG_VIEW_DESKTOP_VIEW"):
        screen = list(self.display.root[view])
        if around_pos is None:
            around_pos = (screen[0] + screen[2] * around_aspect[0],
                          screen[1] + screen[3] * around_aspect[1])
        else:
            around_aspect = ((around_pos[0] - screen[0]) / screen[2],
                             (around_pos[1] - screen[1]) / screen[3])
        screen[2] *= factor
        screen[3] *= factor
        screen[0] = around_pos[0] - screen[2] * around_aspect[0]
        screen[1] = around_pos[1] - screen[3] * around_aspect[1]
        self.display.root[view] = screen
        
    def zoom_to_window(self, event):
        win = self.get_active_window()
        old_view = self.display.root["IG_VIEW_DESKTOP_VIEW"]
        view = list(win["IG_COORDS"])
        view[3] = view[2] * old_view[3] / old_view[2]
        view[1] -= view[3]
        self.display.root["IG_VIEW_DESKTOP_VIEW_ANIMATE"] = view
        self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", self.display.root, "IG_VIEW_DESKTOP_VIEW", .5)

    def zoom_to_more_windows(self, event):
        view = list(self.display.root["IG_VIEW_DESKTOP_VIEW"])
        vx = view[0] + view[2]/2.
        vy = view[1] + view[3]/2.
        
        windows = []
        for child in self.display.root.query_tree().children:
            if child.get_attributes().map_state != Xlib.X.IsViewable:
                continue
            
            child = child.find_client_window()
            if not child: continue
            coords = child["IG_COORDS"]

            # Margins to not get stuck due to rounding errors of
            # windows that sit right on the edge...
            marginx = view[2] / 100
            marginy = view[3] / 100
            if (    coords[0] + marginx >= view[0]
                and coords[0] + coords[2] - marginx <= view[0] + view[2]
                and coords[1] - coords[3] + marginy >= view[1]
                and coords[1] - marginy <= view[1] + view[3]):
                continue
            
            x = coords[0] + coords[2]/2.
            y = coords[1] - coords[3]/2.

            d = math.sqrt((x-vx)**2+(y-vy)**2)
            windows.append((d, coords, child))

        if not windows:
            return

        windows.sort(key=lambda a: a[0])
        d, window, w = windows[0]
        InfiniteGlass.DEBUG("window", "Next window %s/%s[%s] @ %s\n" % (w.get("WM_NAME", None), w.get("WM_CLASS", None), w.__window__(), window))
        
        ratio = view[2] / view[3]
        
        xs = [view[0], view[0] + view[2], window[0], window[0]+window[2]]
        ys = [view[1], view[1] + view[3], window[1], window[1]-window[3]]

        view = [min(xs), min(ys), max(xs) - min(xs), max(ys) - min(ys)]

        InfiniteGlass.DEBUG("view", "View before aspect ratio corr %s\n" % (view,))
        if view[2] / ratio > view[3]:
            view[3] = view[2] / ratio
        else:
            view[2]  = ratio * view[3]
        InfiniteGlass.DEBUG("view", "View %s\n" % (view,))
        self.display.root["IG_VIEW_DESKTOP_VIEW_ANIMATE"] = view
        self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", self.display.root, "IG_VIEW_DESKTOP_VIEW", .5)
        
    def zoom_in(self, event):
        self.zoom(1/1.1)

    def zoom_out(self, event):
        self.zoom(1.1)

class PanMode(Mode):
    def __init__(self, **kw):
        Mode.__init__(self, **kw)
        self.x = 0
        self.y = 0
        self.orig_view = self.display.root["IG_VIEW_DESKTOP_VIEW"]
        self.size = self.display.root["IG_VIEW_DESKTOP_SIZE"]
        
    def pan(self, event):
        self.x += event["XK_Left"] - event["XK_Right"];
        self.y += event["XK_Up"] - event["XK_Down"];

        space_orig = view_to_space(self.orig_view, self.size, 0, 0)
        space = view_to_space(self.orig_view, self.size, self.x, self.y)

        view = list(self.orig_view)
        view[0] = self.orig_view[0] - (space[0] - space_orig[0])
        view[1] = self.orig_view[1] - (space[1] - space_orig[1])
        self.display.root["IG_VIEW_DESKTOP_VIEW"] = view
            
    def pan_mouse(self, event):
        space_orig = view_to_space(self.orig_view, self.size, self.first_event.root_x, self.first_event.root_y)
        space = view_to_space(self.orig_view, self.size, event.root_x, event.root_y)

        view = list(self.orig_view)
        view[0] = self.orig_view[0] - (space[0] - space_orig[0])
        view[1] = self.orig_view[1] - (space[1] - space_orig[1])

        self.display.root["IG_VIEW_DESKTOP_VIEW"] = view

class ItemZoomMode(Mode):
    def enter(self):
        self.window = self.get_event_window(self.first_event)
        if not self.window or self.window == self.display.root:
            return False
        return True
    
    def zoom_1_1_to_sreen(self, event):
        size = self.display.root["IG_VIEW_DESKTOP_SIZE"]
        coords = self.window["IG_COORDS"]
        screen = self.display.root["IG_VIEW_DESKTOP_VIEW"]

        geom = [int(size[0] * coords[2]/screen[2]),
                int(size[1] * coords[3]/screen[3])]

        self.window["IG_SIZE_ANIMATE"] = geom
        self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", self.window, "IG_SIZE", .5)

    def zoom_1_1_to_window(self, event):
        winsize = self.window["IG_SIZE"]
        size = self.display.root["IG_VIEW_DESKTOP_SIZE"]
        coords = self.window["IG_COORDS"]
        screen = list(self.display.root["IG_VIEW_DESKTOP_VIEW"])

        screen[2] = size[0] * coords[2]/winsize[0]
        screen[3] = size[1] * coords[3]/winsize[1]
        screen[0] = coords[0] - (screen[2] - coords[2]) / 2.
        screen[1] = coords[1] - (screen[3] + coords[3]) / 2.

        self.display.root["IG_VIEW_DESKTOP_VIEW_ANIMATE"] = screen
        self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", self.display.root, "IG_VIEW_DESKTOP_VIEW", .5)

    def zoom_in(self, event):
        self.window["IG_SIZE"] = [int(item * 1/1.1) for item in self.window["IG_SIZE"]]

    def zoom_out(self, event):
        self.window["IG_SIZE"] = [int(item * 1.1) for item in self.window["IG_SIZE"]]
    
class ItemPanMode(Mode):
    def enter(self):
        self.window = self.get_event_window(self.first_event)
        if not self.window or self.window == self.display.root:
            pop(self.display)
            push_by_name(self.display, "pan", first_event=self.first_event, last_event=self.last_event)
            return True
        self.x = 0
        self.y = 0
        self.orig_coords = self.window["IG_COORDS"]
        # FIXME: Get the right view...
        self.orig_view = self.display.root["IG_VIEW_DESKTOP_VIEW"]
        self.size = self.display.root["IG_VIEW_DESKTOP_SIZE"]
        return True
    
    def pan(self, event):
        self.x += event["XK_Right"] - event["XK_Left"]
        self.y += event["XK_Down"] - event["XK_Up"]

        space_orig = view_to_space(self.orig_view, self.size, 0, 0)
        space = view_to_space(self.orig_view, self.size, self.x, self.y)

        coords = list(self.orig_coords)
        coords[0] =  self.orig_coords[0] + (space[0] - space_orig[0])
        coords[1] =  self.orig_coords[1] + (space[1] - space_orig[1])

        self.window["IG_COORDS"] = coords

    def pan_mouse(self, event):
        space_orig = view_to_space(self.orig_view, self.size, self.first_event.root_x, self.first_event.root_y)
        space = view_to_space(self.orig_view, self.size, event.root_x, event.root_y)

        coords = list(self.orig_coords)
        coords[0] =  self.orig_coords[0] + (space[0] - space_orig[0])
        coords[1] =  self.orig_coords[1] + (space[1] - space_orig[1])

        self.window["IG_COORDS"] = coords

def main(*arg, **kw):
    global config

    configpath = os.environ.get("GLASS_INPUT_CONFIG", "~/.config/glass/input.json")
    if configpath:
        configpath = os.path.expanduser(configpath)
        
        configdirpath = os.path.dirname(config)
        if not os.path.exists(configdirpath):
            os.makedirs(configdirpath)

        if not os.path.exists(configpath):
            with pkg_resources.resource_stream("glass_input", "config.json") as inf:
                with open(configpath, "wb") as outf:
                    outf.write(inf.read())
    
        with open(configpath) as f:
            config = json.load(f)
    else:
        with pkg_resources.resource_stream("glass_input", "config.json") as f:
            config = json.load(f)
        
    with InfiniteGlass.Display() as display:

        extension_info = display.query_extension('XInputExtension')
        xinput_major = extension_info.major_opcode
        version_info = display.xinput_query_version()
        print('Found XInput version %u.%u' % (
         version_info.major_version,
         version_info.minor_version,
        ))
        
        font = display.open_font('cursor')
        display.input_cursor = font.create_glyph_cursor(
            font, Xlib.Xcursorfont.box_spiral, Xlib.Xcursorfont.box_spiral+1,
            (65535, 65535, 65535), (0, 0, 0))

        display.animate_window = -1
        @display.root.require("IG_ANIMATE")
        def animate_window(root, win):
            display.animate_window = win

        @display.eventhandlers.append
        def handle(event):
            if display.animate_window == -1:
                return False
            return handle_event(display, event)

        push_by_name(display, "base")

        display.root.xinput_select_events([
          (Xlib.ext.xinput.AllDevices, Xlib.ext.xinput.RawMotionMask),
        ])
        
        InfiniteGlass.DEBUG("init", "Input handler started\n")

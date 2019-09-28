import InfiniteGlass
import Xlib.X
import Xlib.Xcursorfont
import Xlib.keysymdef.miscellany
import numpy
import os

debug_events = False
debug_property_notify = False
debug_modes = False

def push(display, Mode, **kw):
    InfiniteGlass.DEBUG("modes.push", "PUSH %s\n" % Mode)
    if not hasattr(display, "input_stack"):
        display.input_stack = []
    display.input_stack.append(Mode(display=display, **kw))

def pop(display):
    res = display.input_stack.pop()
    InfiniteGlass.DEBUG("modes.pop", "POP %s\n" % res)
    res.exit()
    return res

def handle_event(display, event):
    is_propnot = not (event == "PropertyNotify" and event.window == display.notify_motion_window)
    cat = "event" if is_propnot else "event.prop"
    InfiniteGlass.DEBUG(cat, "HANDLE %s\n" % event)
    for i in range(len(display.input_stack)-1, -1, -1):
        mode = display.input_stack[i]
        if mode.handle(event):
            InfiniteGlass.DEBUG(cat, "        BY %s %s\n" % (i, mode))
            return True
    InfiniteGlass.DEBUG(cat, "        UNHANDLED\n")
    return False

def view_to_space(screen, size, screenx, screeny):
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
    
    def exit(self):
        pass

    def handle(self, event):
        pass

    def get_active_window(self):
        try:
            return self.display.notify_motion_window["IG_ACTIVE_WINDOW"]
        except (KeyError, AttributeError) as e:
            print("Unable to get active window", e)
            return None
        
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

    def handle(self, event):
        if event == "PropertyNotify" and event.window == self.display.notify_motion_window:
            win = self.get_active_window()
            if win and win != self.display.root:
                win.set_input_focus(Xlib.X.RevertToNone, Xlib.X.CurrentTime)
        elif (   (event == "ButtonPress" and event["Mod4Mask"])
              or (event == "KeyPress" and event["Mod4Mask"])):
            push(self.display, GrabbedMode)
            handle_event(self.display, event)
        return True

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

    keymap = {
        "KeyRelease": "pop",
        "ButtonRelease": "pop",
        "KeyPress,XK_space": "rofi",
        "KeyPress,XK_Escape": "toggle_overlay",
        "KeyPress,XK_F1": "send_debug",
        "KeyPress,XK_F4": "send_close",
        "KeyPress,XK_F5": "send_sleep",
        "KeyPress,ShiftMask,XK_Home": "zoom_1_1_1",
        "KeyPress,XK_Home": "zoom_home",
        "ButtonPress,Mod1Mask,4": "push_item_zoom",
        "ButtonPress,Mod1Mask,5": "push_item_zoom",
        "KeyPress,Mod1Mask,XK_Next": "push_item_zoom",
        "KeyPress,Mod1Mask,XK_Prior": "push_item_zoom",
        "ButtonPress,4": "push_zoom",
        "ButtonPress,5": "push_zoom",
        "KeyPress,XK_Next": "push_zoom",
        "KeyPress,XK_Prior": "push_zoom",
        "ButtonPress,2": "push_pan",
        "ButtonPress,1,ControlMask": "push_pan",
        "KeyPress,ControlMask,XK_Up": "push_pan",
        "KeyPress,ControlMask,XK_Down": "push_pan",
        "KeyPress,ControlMask,XK_Left": "push_pan",
        "KeyPress,ControlMask,XK_Right": "push_pan",
        "ButtonPress,1": "push_item_pan",
        "KeyPress,XK_Up": "push_item_pan",
        "KeyPress,XK_Down": "push_item_pan",
        "KeyPress,XK_Left": "push_item_pan",
        "KeyPress,XK_Right": "push_item_pan"
    }
    
    def handle(self, event):
        for key, value in self.keymap.items():
            if event == key.split(","):
                getattr(self, value)(event)
                return True
        return False
    
    def pop(self, event):
        pop(self.display)

    def rofi(self, event):
        os.system('rofi -show drun -font "DejaVu Sans 18" -show-icons &')

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

    def push_item_zoom(self, event):
        if event == "ButtonPress":
            win = self.get_active_window()
        else:
            win = self.display.get_input_focus().focus
        if win and win != self.display.root:
            push(self.display, ItemZoomMode, window=win, first_event=event, last_event=event)
            handle_event(self.display, event);

    def push_zoom(self, event):
        push(self.display, ZoomMode)
        handle_event(self.display, event)

    def push_pan(self, event):
        push(self.display, PanMode, first_event=event, last_event=event)
        handle_event(self.display, event)

    def push_item_pan(self, event):
        if event == "ButtonPress":
            win = self.get_active_window()
        else:
            win = self.display.get_input_focus().focus
        InfiniteGlass.DEBUG("button", "BUTTON PRESS %s\n" % win)
        if win and win != self.display.root:
            push(self.display, ItemPanMode, window=win, first_event=event, last_event=event)
            handle_event(self.display, event)

    
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
        
    def handle(self, event):
        if event == "KeyRelease":
            pop(self.display)
        elif (   (event == "ButtonRelease" and event[4] and (event["ShiftMask"]))
                 or (event == "KeyPress" and event["XK_Prior"] and (event["ShiftMask"]))):
            win = self.get_active_window()
            old_view = self.display.root["IG_VIEW_DESKTOP_VIEW"]
            view = list(win["IG_COORDS"])
            view[3] = view[2] * old_view[3] / old_view[2]
            view[1] -= view[3]
            self.display.root["IG_VIEW_DESKTOP_VIEW_ANIMATE"] = view
            self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", self.display.root, "IG_VIEW_DESKTOP_VIEW", .5)
        elif (event == "ButtonRelease" and event[5]
                 and (event["ShiftMask"])):
            pass
        elif event == "KeyPress" and event["XK_Prior"]:
            self.zoom(1/1.1)
        elif event == "KeyPress" and event["XK_Next"]: # down -> zoom out
            self.zoom(1.1)
        elif event == "ButtonRelease" and event[4]: # up -> zoom in
            self.zoom(1/1.1) # We should supply around_pos here...
        elif event == "ButtonRelease" and event[5]: # down -> zoom out
            self.zoom(1.1) # We should supply around_pos here...
        return True

class PanMode(Mode):
    def __init__(self, **kw):
        Mode.__init__(self, **kw)
        self.x = 0
        self.y = 0
        self.orig_view = self.display.root["IG_VIEW_DESKTOP_VIEW"]
        self.size = self.display.root["IG_VIEW_DESKTOP_SIZE"]

    def handle(self, event):
        if event == "KeyRelease" or event == "ButtonRelease":
            pop(self.display)
        elif event == "KeyPress":
            self.x += event["XK_Left"] - event["XK_Right"];
            self.y += event["XK_Up"] - event["XK_Down"];
            
            space_orig = view_to_space(self.orig_view, self.size, 0, 0)
            space = view_to_space(self.orig_view, self.size, self.x, self.y)

            view = list(self.orig_view)
            view[0] = self.orig_view[0] - (space[0] - space_orig[0])
            view[1] = self.orig_view[1] - (space[1] - space_orig[1])
            self.display.root["IG_VIEW_DESKTOP_VIEW"] = view
            
        elif self.first_event == "ButtonPress" and event == "MotionNotify":
            space_orig = view_to_space(self.orig_view, self.size, self.first_event.root_x, self.first_event.root_y)
            space = view_to_space(self.orig_view, self.size, event.root_x, event.root_y)

            view = list(self.orig_view)
            view[0] = self.orig_view[0] - (space[0] - space_orig[0])
            view[1] = self.orig_view[1] - (space[1] - space_orig[1])

            self.display.root["IG_VIEW_DESKTOP_VIEW"] = view
        
        return True

class ItemZoomMode(Mode):
    def handle(self, event):
        if event == "KeyRelease":
            pop(self.display)
        elif (   (event == "ButtonRelease" and event["ShiftMask"] and event[4])
              or (event == "KeyPress" and event["ShiftMask"] and event["XK_Prior"])):
            # zoom_window_to_1_to_1_to_screen
            size = self.display.root["IG_VIEW_DESKTOP_SIZE"]
            coords = self.window["IG_COORDS"]
            screen = self.display.root["IG_VIEW_DESKTOP_VIEW"]

            geom = [int(size[0] * coords[2]/screen[2]),
                    int(size[1] * coords[3]/screen[3])]
            
            self.window["IG_SIZE_ANIMATE"] = geom
            self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", self.window, "IG_SIZE", .5)
        elif (   (event == "ButtonRelease" and event["ShiftMask"] and event[5])
              or (event == "KeyPress" and event["ShiftMask"] and event["XK_Next"])):
            # zoom_screen_to_1_to_1_to_window

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
        elif (   (event == "ButtonRelease" and event[4])
              or (event == "KeyPress" and event["XK_Prior"])):
            self.window["IG_SIZE"] = [int(item * 1/1.1) for item in self.window["IG_SIZE"]]
        elif (   (event == "ButtonRelease" and event[5])
              or (event == "KeyPress" and event["XK_Next"])):
            self.window["IG_SIZE"] = [int(item * 1.1) for item in self.window["IG_SIZE"]]
        return True
    
class ItemPanMode(Mode):
    def __init__(self, **kw):
        Mode.__init__(self, **kw)
        self.x = 0
        self.y = 0
        self.orig_coords = self.window["IG_COORDS"]
        # FIXME: Get the right view...
        self.orig_view = self.display.root["IG_VIEW_DESKTOP_VIEW"]
        self.size = self.display.root["IG_VIEW_DESKTOP_SIZE"]

    def handle(self, event):
        if event == "KeyRelease" or event == "ButtonRelease":
            pop(self.display)
        elif event == "KeyPress":
            self.x += event["XK_Right"] - event["XK_Left"]
            self.y += event["XK_Down"] - event["XK_Up"]

            space_orig = view_to_space(self.orig_view, self.size, 0, 0)
            space = view_to_space(self.orig_view, self.size, self.x, self.y)

            coords = list(self.orig_coords)
            coords[0] =  self.orig_coords[0] + (space[0] - space_orig[0])
            coords[1] =  self.orig_coords[1] + (space[1] - space_orig[1])
            
            self.window["IG_COORDS"] = coords

        elif event == "MotionNotify":
            space_orig = view_to_space(self.orig_view, self.size, self.first_event.root_x, self.first_event.root_y)
            space = view_to_space(self.orig_view, self.size, event.root_x, event.root_y)
            
            coords = list(self.orig_coords)
            coords[0] =  self.orig_coords[0] + (space[0] - space_orig[0])
            coords[1] =  self.orig_coords[1] + (space[1] - space_orig[1])
            
            self.window["IG_COORDS"] = coords
        return True

def main(*arg, **kw):
    with InfiniteGlass.Display() as display:
        font = display.open_font('cursor')
        display.input_cursor = font.create_glyph_cursor(
            font, Xlib.Xcursorfont.box_spiral, Xlib.Xcursorfont.box_spiral+1,
            (65535, 65535, 65535), (0, 0, 0))

        display.notify_motion_window = -1
        @display.root.require("IG_NOTIFY_MOTION")
        def notify_motion(root, win):
            win.change_attributes(event_mask = Xlib.X.PropertyChangeMask)
            display.notify_motion_window = win

        display.animate_window = -1
        @display.root.require("IG_ANIMATE")
        def notify_motion(root, win):
            display.animate_window = win

        @display.eventhandlers.append
        def handle(event):
            if display.notify_motion_window == -1 or display.animate_window == -1:
                return False
            return handle_event(display, event)

        push(display, BaseMode)

        InfiniteGlass.DEBUG("init", "Input handler started\n")

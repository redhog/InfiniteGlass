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
    cat = "event.prop" if is_propnot else "event"
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
        elif event == "KeyPress" and event["XK_Super_L"]:
            push(self.display, GrabbedMode)
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

    def handle(self, event):
        if event == "KeyRelease" and event["XK_Super_L"]:
            # Not Mod4Mask here, as we only want to ungrab when the super key itself is released
            pop(self.display)
        elif event == "KeyPress" and event["XK_space"]:
            os.system('rofi -show drun -font "DejaVu Sans 18" -show-icons &')
        elif event == "KeyPress" and event["XK_Escape"]:
            old = self.display.root["IG_VIEW_OVERLAY_VIEW"]
            if old[0] == 0.:
                self.display.root["IG_VIEW_OVERLAY_VIEW_ANIMATE"] = [.2, .2, .6, .6 * old[3] / old[2]]
            else:
                self.display.root["IG_VIEW_OVERLAY_VIEW_ANIMATE"] = [0., 0., 1., old[3] / old[2]]
            self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", self.display.root, "IG_VIEW_OVERLAY_VIEW", .5)
        elif event == "KeyPress" and event["XK_F1"]:
            InfiniteGlass.DEBUG("debug", "SENDING DEBUG\n")
            self.display.root.send(
                self.display.root, "IG_DEBUG",
                event_mask=Xlib.X.StructureNotifyMask|Xlib.X.SubstructureRedirectMask)
        elif event == "KeyPress" and event["XK_F4"]:
            win = self.get_active_window()
            if win and win != self.display.root:
                InfiniteGlass.DEBUG("close", "SENDING CLOSE %s\n" % win)
                win.send(win, "IG_CLOSE", event_mask=Xlib.X.StructureNotifyMask)
        elif event == "KeyPress" and event["XK_F5"]:
            win = self.get_active_window()
            if win and win != self.display.root:
                InfiniteGlass.DEBUG("sleep", "SENDING SLEEP %s\n" % win)
                win.send(win, "IG_SLEEP", event_mask=Xlib.X.StructureNotifyMask)
        elif event == "KeyPress" and event["ShiftMask"] and event["XK_Home"]:
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
        elif event == "KeyPress" and event["XK_Home"]:
            InfiniteGlass.DEBUG("zoom", "ZOOM HOME\n")
            old = self.display.root["IG_VIEW_DESKTOP_VIEW"]
            self.display.root["IG_VIEW_DESKTOP_VIEW_ANIMATE"] = [0., 0., 1., old[3] / old[2]]
            self.display.animate_window.send(self.display.animate_window, "IG_ANIMATE", self.display.root, "IG_VIEW_DESKTOP_VIEW", .5)
        elif (   (event == "ButtonPress" and event["Mod1Mask"] and event[4])
              or (event == "ButtonPress" and event["Mod1Mask"] and event[5])
              or (event == "KeyPress" and event["Mod1Mask"] and event["XK_Next"])
              or (event == "KeyPress" and event["Mod1Mask"] and event["XK_Prior"])):
            if event == "ButtonPress":
                win = self.get_active_window()
            else:
                win = self.display.get_input_focus().focus
            if win and win != self.display.root:
                push(self.display, ItemZoomMode, window=win, first_event=event, last_event=event)
                handle_event(self.display, event);

        elif (   (    event == "ButtonPress"
                         and (   event[4]
                              or event[5]))
                    or (    event == "KeyPress"
                            and (   event["XK_Next"]
                                 or event["XK_Prior"]))):
            push(self.display, ZoomMode)
            handle_event(self.display, event)
        elif (   (event == "ButtonPress" and event[2])
                 or (event == "ButtonPress" and event[1] and event["ControlMask"])
                 or (event == "KeyPress" and event["ControlMask"] and event["XK_Up"])
                 or (event == "KeyPress" and event["ControlMask"] and event["XK_Down"])
                 or (event == "KeyPress" and event["ControlMask"] and event["XK_Left"])
                 or (event == "KeyPress" and event["ControlMask"] and event["XK_Right"])):
            push(self.display, PanMode, first_event=event, last_event=event)
            handle_event(self.display, event)
        elif (   (event == "ButtonPress" and event[1])
                  or (event == "KeyPress" and event["XK_Up"])
                  or (event == "KeyPress" and event["XK_Down"])
                  or (event == "KeyPress" and event["XK_Left"])
                  or (event == "KeyPress" and event["XK_Right"])):
            if event == "ButtonPress":
                win = self.get_active_window()
            else:
                win = self.display.get_input_focus().focus
            InfiniteGlass.DEBUG("button", "BUTTON PRESS %s\n" % win)
            if win and win != self.display.root:
                push(self.display, ItemPanMode, window=win, first_event=event, last_event=event)
                handle_event(self.display, event)
        return True

    
class ZoomMode(Mode):
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
            screen = list(self.display.root["IG_VIEW_DESKTOP_VIEW"])
            screen[2] *= 1/1.1
            screen[3] *= 1/1.1
            self.display.root["IG_VIEW_DESKTOP_VIEW"] = screen
        elif event == "KeyPress" and event["XK_Next"]: # down -> zoom out
            screen = list(self.display.root["IG_VIEW_DESKTOP_VIEW"])
            screen[2] *= 1.1
            screen[3] *= 1.1
            self.display.root["IG_VIEW_DESKTOP_VIEW"] = screen
        elif event == "ButtonRelease" and event[4]: # up -> zoom in
            screen = list(self.display.root["IG_VIEW_DESKTOP_VIEW"])
            screen[2] *= 1/1.1
            screen[3] *= 1/1.1
            self.display.root["IG_VIEW_DESKTOP_VIEW"] = screen
        elif event == "ButtonRelease" and event[5]: # down -> zoom out
            screen = list(self.display.root["IG_VIEW_DESKTOP_VIEW"])
            screen[2] *= 1.1
            screen[3] *= 1.1
            self.display.root["IG_VIEW_DESKTOP_VIEW"] = screen
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

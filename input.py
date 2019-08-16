import InfiniteGlass
import Xlib.X
import Xlib.Xcursorfont
import Xlib.keysymdef.miscellany
import numpy

debug_events = False
debug_property_notify = False
debug_modes = False

def push(display, Mode, **kw):
    if debug_modes: print("PUSH", Mode)
    if not hasattr(display, "input_stack"):
        display.input_stack = []
    display.input_stack.append(Mode(display=display, **kw))

def pop(display):
    res = display.input_stack.pop()
    if debug_modes: print("POP", res)
    res.exit()
    return res

def handle_event(display, event):
    is_propnot = not (event == "PropertyNotify" and event.window == display.notify_motion_window)
    if debug_property_notify and (is_propnot or debug_property_notify):
        print("    HANDLE", event)
    for i in range(len(display.input_stack)-1, -1, -1):
        mode = display.input_stack[i]
        if mode.handle(event):
            if debug_property_notify and (is_propnot or debug_property_notify):
                print("        BY", i, mode)
            return True
    if debug_property_notify and (is_propnot or debug_property_notify):
        print("        UNHANDLED")    
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
            self.display.root.grab_key(display.keycode("XK_Super_L"),
                                       mod, False, Xlib.X.GrabModeAsync, Xlib.X.GrabModeAsync)
            self.display.root.grab_button(
                Xlib.X.AnyButton, Xlib.X.Mod4Mask | mod, False,
                Xlib.X.ButtonPressMask | Xlib.X.ButtonReleaseMask | Xlib.X.PointerMotionMask,
                Xlib.X.GrabModeAsync, Xlib.X.GrabModeAsync, self.display.root, cursor)           
        self.display.root["IG_VIEW_OVERLAY_VIEW"] = [.2, .2, .6, .6*.75]

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
            Xlib.X.GrabModeAsync, Xlib.X.GrabModeAsync, self.display.root, cursor, Xlib.X.CurrentTime)
        self.display.root.grab_keyboard(False, Xlib.X.GrabModeAsync, Xlib.X.GrabModeAsync, Xlib.X.CurrentTime)

    def exit(self):
        self.display.ungrab_pointer(Xlib.X.CurrentTime)
        self.display.ungrab_keyboard(Xlib.X.CurrentTime)

    def handle(self, event):
        if event == "KeyRelease" and event["XK_Super_L"]:
            # Not Mod4Mask here, as we only want to ungrab when the super key itself is released
            pop(self.display)
        elif event == "KeyPress" and event["XK_Escape"]:
            old = self.display.root["IG_VIEW_OVERLAY_VIEW"]
            if old[0] == 0.:
                self.display.root["IG_VIEW_OVERLAY_VIEW_ANIMATE"] = [.2, .2, .6, .6*.75]
            else:
                self.display.root["IG_VIEW_OVERLAY_VIEW_ANIMATE"] = [0., 0., 1., .75]
            display.animate_window.send(display.animate_window, "IG_ANIMATE", self.display.root, "IG_VIEW_OVERLAY_VIEW", .5)
        elif event == "KeyPress" and event["ShiftMask"] and event["XK_Home"]:
            win = self.get_active_window()
            if win and win != self.display.root:
                # zoom_screen_to_window_and_window_to_screen

                print("zoom_screen_to_window_and_window_to_screen")
                wingeom = win.get_geometry()
                viewgeom = self.display.root["IG_VIEW_DESKTOP_SIZE"]
                coords = list(win["IG_COORDS"])
                screen = list(self.display.root["IG_VIEW_DESKTOP_VIEW"])

                geom = [wingeom.x, wingeom.y, viewgeom[0], viewgeom[1]]

                coords[3] = (viewgeom[1] * coords[2]) / viewgeom[0]

                screen[2] = coords[2]
                screen[3] = coords[3]
                screen[0] = coords[0]
                screen[1] = coords[1] - screen[3]

                print("    screen=%s geom=%s" % (screen, geom))
                
                win["IG_COORDS_ANIMATE"] = coords
                win["__GEOMETRY__ANIMATE"] = geom
                self.display.root["IG_VIEW_DESKTOP_VIEW_ANIMATE"] = screen
                display.animate_window.send(display.animate_window, "IG_ANIMATE", win, "IG_COORDS", .5)
                display.animate_window.send(display.animate_window, "IG_ANIMATE", win, "__GEOMETRY__", .5)
                display.animate_window.send(display.animate_window, "IG_ANIMATE", self.display.root, "IG_VIEW_DESKTOP_VIEW", .5)
                display.flush()
                display.sync()
        elif event == "KeyPress" and event["XK_Home"]:
            print("ZOOM HOME")
            self.display.root["IG_VIEW_DESKTOP_VIEW_ANIMATE"] = [0., 0., 1., .75]
            display.animate_window.send(display.animate_window, "IG_ANIMATE", self.display.root, "IG_VIEW_DESKTOP_VIEW", .5)
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
            display.animate_window.send(display.animate_window, "IG_ANIMATE", self.display.root, "IG_VIEW_DESKTOP_VIEW", .5)
        elif (event == "ButtonRelease" and event[5]
                 and (event["ShiftMask"])):
            pass
        elif event == "KeyPress" and event["XK_Prior"]:
            self.display.root.send(self.display.root,
                                   "IG_ZOOM", "IG_LAYER_DESKTOP", 1 / 1.1, -1, -1,
                                   event_mask=Xlib.X.SubstructureNotifyMask|Xlib.X.SubstructureRedirectMask)
        elif event == "KeyPress" and event["XK_Next"]: # down -> zoom out
            self.display.root.send(self.display.root,
                                   "IG_ZOOM", "IG_LAYER_DESKTOP", 1.1, -1, -1,
                                   event_mask=Xlib.X.SubstructureNotifyMask|Xlib.X.SubstructureRedirectMask)
        elif event == "ButtonRelease" and event[4]: # up -> zoom in
            self.display.root.send(self.display.root,
                                   "IG_ZOOM", "IG_LAYER_DESKTOP", 1/1.1, event.root_x, event.root_y,
                                   event_mask=Xlib.X.SubstructureNotifyMask|Xlib.X.SubstructureRedirectMask)
        elif event == "ButtonRelease" and event[5]: # down -> zoom out
            self.display.root.send(self.display.root,
                                   "IG_ZOOM", "IG_LAYER_DESKTOP", 1.1, event.root_x, event.root_y,
                                   event_mask=Xlib.X.SubstructureNotifyMask|Xlib.X.SubstructureRedirectMask)
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
            wingeom = self.window.get_geometry()
            viewgeom = self.display.root["IG_VIEW_DESKTOP_SIZE"]
            coords = self.window["IG_COORDS"]
            screen = self.display.root["IG_VIEW_DESKTOP_VIEW"]

            geom = [wingeom.x, wingeom.y,
                    int(viewgeom[0] * coords[2]/screen[2]),
                    int(viewgeom[1] * coords[3]/screen[3])]
            
            self.window["__GEOMETRY__ANIMATE"] = geom
            display.animate_window.send(display.animate_window, "IG_ANIMATE", self.window, "__GEOMETRY__", .5)
        elif (   (event == "ButtonRelease" and event["ShiftMask"] and event[5])
              or (event == "KeyPress" and event["ShiftMask"] and event["XK_Next"])):
            # zoom_screen_to_1_to_1_to_window

            wingeom = self.window.get_geometry()
            viewgeom = self.display.root["IG_VIEW_DESKTOP_SIZE"]
            coords = self.window["IG_COORDS"]
            screen = list(self.display.root["IG_VIEW_DESKTOP_VIEW"])

            screen[2] = viewgeom[0] * coords[2]/wingeom.width
            screen[3] = viewgeom[1] * coords[3]/wingeom.height
            screen[0] = coords[0] - (screen[2] - coords[2]) / 2.
            screen[1] = coords[1] - (screen[3] + coords[3]) / 2.

            self.display.root["IG_VIEW_DESKTOP_VIEW_ANIMATE"] = screen
            display.animate_window.send(display.animate_window, "IG_ANIMATE", self.display.root, "IG_VIEW_DESKTOP_VIEW", .5)
        elif (   (event == "ButtonRelease" and event[4])
              or (event == "KeyPress" and event["XK_Prior"])):
            geom = self.window.get_geometry()
            width = int(geom.width * 1/1.1)
            height = int(geom.height * 1/1.1)
            self.window.configure(width = width, height = height)
        elif (   (event == "ButtonRelease" and event[5])
              or (event == "KeyPress" and event["XK_Next"])):
            geom = self.window.get_geometry()
            width = int(geom.width * 1.1)
            height = int(geom.height * 1.1)
            self.window.configure(width = width, height = height)
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
    
with InfiniteGlass.Display() as display:
    font = display.open_font('cursor')
    cursor = font.create_glyph_cursor(font, Xlib.Xcursorfont.box_spiral, Xlib.Xcursorfont.box_spiral+1, (65535, 65535, 65535), (0, 0, 0))
    
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

import InfiniteGlass
import Xlib.X
import Xlib.Xcursorfont
import Xlib.keysymdef.miscellany
import numpy

def push(display, Mode, **kw):
    print("PUSH", Mode)
    if not hasattr(display, "input_stack"):
        display.input_stack = []
    display.input_stack.append(Mode(display=display, **kw))

def pop(display):
    res = display.input_stack.pop()
    res.exit()
    return res

def handle_event(display, event):
    for i in range(len(display.input_stack)-1, -1, -1):
        if display.input_stack[i].handle(event):
            return True
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
        except (KeyError, AttributeError):
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

    def handle(self, event):
        if event == "PropertyNotify" and event.window == self.display.notify_motion_window:
            win = self.get_active_window()
            if win and win != self.display.root:
                win.set_input_focus(Xlib.X.RevertToNone, Xlib.X.CurrentTime)
        elif event == "KeyPress" and event["ShiftMask"] and event["XK_Home"]:
            win = self.get_active_window()
            if win and win != self.display.root:
                self.self.display.root.send(self.display.root,
                                            "IG_ZOOM_TO_WINDOW", "IG_LAYER_DESKTOP", win, 1,
                                            event_mask=Xlib.X.SubstructureNotifyMask|Xlib.X.SubstructureRedirectMask)
        elif event == "KeyPress" and event["XK_Home"]:
            self.display.root.send(self.display.root, "IG_ZOOM", "IG_LAYER_DESKTOP", -1.0, -1, -1)
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
                 or (event == "ButtonPress" and event[1] and event["ControlMask)"])
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
                win = self.self.display.get_input_focus().focus
            if win and win != self.display.root:
                push(self.display, ItemPanMode, window=win, first_even=event, last_event=event)
                handle_event(self.display, event)
        return True
                
class ZoomMode(Mode):
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
        print("ZoomMode", event)
        if event == "KeyRelease":
            pop(self.display);
        elif (   (event == "ButtonRelease" and event[4] and (event["ShiftMask"]))
                 or (event == "KeyPress" and event["XK_Prior"] and (event["ShiftMask"]))):
            # shift up -> zoom in to window
            self.display.root.send(self.display.root,
                                   "IG_ZOOM_TO_WINDOW", "IG_LAYER_DESKTOP", self.get_active_window(), 0,
                                   event_mask=Xlib.X.SubstructureNotifyMask|Xlib.X.SubstructureRedirectMask)
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
        self.display.root.grab_pointer(
            False, Xlib.X.ButtonPressMask | Xlib.X.ButtonReleaseMask | Xlib.X.PointerMotionMask,
            Xlib.X.GrabModeAsync, Xlib.X.GrabModeAsync, self.display.root, cursor, Xlib.X.CurrentTime)
        self.display.root.grab_keyboard(False, Xlib.X.GrabModeAsync, Xlib.X.GrabModeAsync, Xlib.X.CurrentTime)
        self.x = 0
        self.y = 0
        self.orig_view = self.display.root["IG_VIEW_DESKTOP_VIEW"]
        self.size = self.display.root["IG_VIEW_DESKTOP_SIZE"]
        
    def exit(self):
        self.display.ungrab_pointer(Xlib.X.CurrentTime)
        self.display.ungrab_keyboard(Xlib.X.CurrentTime)

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
            space_orig = view_to_space(
                self.orig_view, self.size,
                self.first_event.root_x, self.first_event.root_y)
            space = view_to_space(
                self.orig_view, self.size,
                event.root_x, event.root_y)

            view = list(self.orig_view)
            view[0] = self.orig_view[0] - (space[0] - space_orig[0])
            view[1] = self.orig_view[1] - (space[1] - space_orig[1])

            self.display.root["IG_VIEW_DESKTOP_VIEW"] = view
        
        return True

class ItemZoomMode(Mode):
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
        return True
    
class ItemPanMode(Mode):
    def handle(self, event):
        pop(self.display)
        return True
    
with InfiniteGlass.Display() as display:
    font = display.open_font('cursor')
    cursor = font.create_glyph_cursor(font, Xlib.Xcursorfont.box_spiral, Xlib.Xcursorfont.box_spiral+1, (65535, 65535, 65535), (0, 0, 0))
    
    display.notify_motion_window = -1
    @display.root.require("IG_NOTIFY_MOTION")
    def notify_motion(root, win):
        win.change_attributes(event_mask = Xlib.X.PropertyChangeMask)
        display.notify_motion_window = win
    
    @InfiniteGlass.eventhandlers.append
    def handle(event):
        return handle_event(display, event)

    push(display, BaseMode)

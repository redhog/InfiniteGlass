import InfiniteGlass
import Xlib.X
import Xlib.Xcursorfont
import Xlib.keysymdef.miscellany

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
        if event.type == Xlib.X.PropertyNotify and event.window == self.display.notify_motion_window:
            win = self.get_active_window()
            if win and win != self.display.root:
                win.set_input_focus(Xlib.X.RevertToNone, Xlib.X.CurrentTime)
        elif event.type == Xlib.X.KeyPress and event.state & Xlib.X.ShiftMask and event.detail == self.display.keycode("XK_Home"):
            win = self.get_active_window()
            if win and win != self.display.root:
                self.self.display.root.send(self.display.root,
                                            "IG_ZOOM_TO_WINDOW", "IG_LAYER_DESKTOP", win, 1,
                                            event_mask=Xlib.X.SubstructureNotifyMask|Xlib.X.SubstructureRedirectMask)
        elif event.type == Xlib.X.KeyPress and event.detail == self.display.keycode("XK_Home"):
            self.display.root.send(self.display.root, "IG_ZOOM", "IG_LAYER_DESKTOP", -1.0, -1, -1)
        elif (   (event.type == Xlib.X.ButtonPress and event.state & Xlib.X.Mod1Mask and event.detail == 4)
                 or (event.type == Xlib.X.ButtonPress and event.state & Xlib.X.Mod1Mask and event.detail == 5)
                 or (event.type == Xlib.X.KeyPress and event.state & Xlib.X.Mod1Mask and event.detail == self.display.keycode("XK_Next"))
                 or (event.type == Xlib.X.KeyPress and event.state & Xlib.X.Mod1Mask and event.detail == self.display.keycode("XK_Prior"))):
            if event.type == Xlib.X.ButtonPress:
                win = self.get_active_window()
            else:
                win = self.display.get_input_focus().focus
            if win and win != self.display.root:
                push(self.display, ItemZoomMode, window=win, first_event=event, last_event=event)
                handle_event(self.display, event);

        elif (   (    event.type == Xlib.X.ButtonPress
                         and (   event.detail == 4
                              or event.detail == 5))
                    or (    event.type == Xlib.X.KeyPress
                            and (   event.detail == self.display.keycode("XK_Next")
                                 or event.detail == self.display.keycode("XK_Prior")))):
            push(self.display, ZoomMode)
            handle_event(self.display, event)
        elif (   (event.type == Xlib.X.ButtonPress and event.detail == 2)
                 or (event.type == Xlib.X.ButtonPress and event.detail == 1 and event.state & Xlib.X.ControlMask)
                 or (event.type == Xlib.X.KeyPress and event.state & Xlib.X.ControlMask and event.detail == self.display.keycode("XK_Up"))
                 or (event.type == Xlib.X.KeyPress and event.state & Xlib.X.ControlMask and event.detail == self.display.keycode("XK_Down"))
                 or (event.type == Xlib.X.KeyPress and event.state & Xlib.X.ControlMask and event.detail == self.display.keycode("XK_Left"))
                 or (event.type == Xlib.X.KeyPress and event.state & Xlib.X.ControlMask and event.detail == self.display.keycode("XK_Right"))):
            push(self.display, PanMode, first_event=event, last_event=event)
            handle_event(self.display, event)
        elif (   (event.type == Xlib.X.ButtonPress and event.detail == 1)
                  or (event.type == Xlib.X.KeyPress and event.detail == self.display.keycode("XK_Up"))
                  or (event.type == Xlib.X.KeyPress and event.detail == self.display.keycode("XK_Down"))
                  or (event.type == Xlib.X.KeyPress and event.detail == self.display.keycode("XK_Left"))
                  or (event.type == Xlib.X.KeyPress and event.detail == self.display.keycode("XK_Right"))):
            if event.type == Xlib.X.ButtonPress:
                win = self.get_active_window()
            else:
                win = self.self.display.get_input_focus().focus
            if win and win != self.display.root:
                push(self.display, ItemPanMode, window=win, first_even=event, last_event=event)
                handle_event(self.display, event)

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
        if event.type == Xlib.X.KeyRelease:
            pop(self.display);
        elif (   (event.type == Xlib.X.ButtonRelease and event.detail == 4 and (event.state & Xlib.X.ShiftMask))
                 or (event.type == Xlib.X.KeyPress and event.detail == self.display.keycode("XK_Prior") and (event.state & Xlib.X.ShiftMask))):
            # shift up -> zoom in to window
            self.display.root.send(self.display.root,
                                   "IG_ZOOM_TO_WINDOW", "IG_LAYER_DESKTOP", self.get_active_window(), 0,
                                   event_mask=Xlib.X.SubstructureNotifyMask|Xlib.X.SubstructureRedirectMask)
        elif (event.type == Xlib.X.ButtonRelease and event.detail == 5
                 and (event.state & Xlib.X.ShiftMask)):
            pass
        elif event.type == Xlib.X.KeyPress and event.detail == self.display.keycode("XK_Prior"):
            self.display.root.send(self.display.root,
                                   "IG_ZOOM", "IG_LAYER_DESKTOP", 1 / 1.1, -1, -1,
                                   event_mask=Xlib.X.SubstructureNotifyMask|Xlib.X.SubstructureRedirectMask)
        elif event.type == Xlib.X.KeyPress and event.detail == self.display.keycode("XK_Next"): # down -> zoom out
            self.display.root.send(self.display.root,
                                   "IG_ZOOM", "IG_LAYER_DESKTOP", 1.1, -1, -1,
                                   event_mask=Xlib.X.SubstructureNotifyMask|Xlib.X.SubstructureRedirectMask)
        elif event.type == Xlib.X.ButtonRelease and event.detail == 4: # up -> zoom in
            self.display.root.send(self.display.root,
                                   "IG_ZOOM", "IG_LAYER_DESKTOP", 1/1.1, event.root_x, event.root_y,
                                   event_mask=Xlib.X.SubstructureNotifyMask|Xlib.X.SubstructureRedirectMask)
        elif event.type == Xlib.X.ButtonRelease and event.detail == 5: # down -> zoom out
            self.display.root.send(self.display.root,
                                   "IG_ZOOM", "IG_LAYER_DESKTOP", 1.1, event.root_x, event.root_y,
                                   event_mask=Xlib.X.SubstructureNotifyMask|Xlib.X.SubstructureRedirectMask)
        return True

class PanMode(Mode): pass
class ItemZoomMode(Mode): pass
class ItemPanMode(Mode): pass

    
with InfiniteGlass.Display() as display:
    font = display.open_font('cursor')
    cursor = font.create_glyph_cursor(font, Xlib.Xcursorfont.box_spiral, Xlib.Xcursorfont.box_spiral+1, (65535, 65535, 65535), (0, 0, 0))
    
    push(display, BaseMode)

    @display.root.require("IG_NOTIFY_MOTION")
    def notify_motion(root, win):
        win.change_attributes(event_mask = Xlib.X.PropertyChangeMask)
        display.notify_motion_window = win
    
    @InfiniteGlass.eventhandlers.append
    def handle(event):
        return handle_event(display, event)


            

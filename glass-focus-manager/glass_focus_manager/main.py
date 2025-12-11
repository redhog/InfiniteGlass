"""
Minimal focus-handling to be ICCM and EWMH compliant 

Behaviors implemented:
 - Listen for _NET_ACTIVE_WINDOW client messages (EWMH) on the root window.
 - Check client-supplied timestamp and simple focus-stealing prevention.
 - Update root _NET_ACTIVE_WINDOW property when activating a window.
 - Respect ICCCM WM_HINTS.input and WM_PROTOCOLS/WM_TAKE_FOCUS:
     - if WM_TAKE_FOCUS is listed -> send WM_TAKE_FOCUS client message to app
     - otherwise call SetInputFocus on the target window (XSetInputFocus)
 - Handles some edge cases (override-redirect, unmapped windows, timestamp == 0)
"""

import Xlib.X
import InfiniteGlass
import sys
import traceback
import click



# from Xlib import X, XK, display, Xatom, protocol
# from Xlib.protocol import event
# import time
# import struct


class Manager(object):
    # Simple policy parameters
    ACCEPT_OLD_TIMESTAMP = False   # if False, ignore activation requests older than last_user_timestamp
    ALLOW_ZERO_TIMESTAMP = True    # if True: treat data[1]==0 as "unknown", fall back to allow or other heuristics
    
    def __init__(self, display, **kw):
        self.display = display
        root = display.root

        # Keep track of last user input timestamp (ms since server epoch).
        # A real WM should update this from ButtonPress/KeyPress events and
        # possibly from _NET_WM_USER_TIME_WINDOW client property updates.
        self.last_user_timestamp = 0
        
        # Claim WM_S0 selection (ICCCM requirement)
        # root.set_selection_owner(display.intern_atom("WM_S0"), Xlib.X.CurrentTime)
        # owner = display.get_selection_owner(display.intern_atom("WM_S0"))
        # print("WM_S0 owner:", owner.id if owner else None)

        # Supporting WM check window (EWMH requirement)
        support_win = root.create_window(0, 0, 1, 1, 0, Xlib.X.CopyFromParent)
        support_win["_NET_SUPPORTING_WM_CHECK"] = support_win
        support_win["_NET_WM_NAME"] = ("UTF8_STRING", ["InfiniteGlass".encode("utf-8")])        
        root["_NET_SUPPORTING_WM_CHECK"] = support_win
        root["_NET_SUPPORTED"] = ["_NET_ACTIVE_WINDOW"]
        
        root.change_attributes(
            event_mask=Xlib.X.ButtonPressMask | Xlib.X.KeyPressMask | Xlib.X.SubstructureNotifyMask | Xlib.X.SubstructureNotifyMask | Xlib.X.PropertyChangeMask)

        @root.on(mask="SubstructureNotifyMask", client_type="_NET_ACTIVE_WINDOW")
        def ClientMessage(win, event):
            self.handle_net_active_window(event)

        @root.on(atom="_NET_WM_USER_TIME")
        def PropertyNotify(win, event):
            try:
                self.update_last_user_timestamp(int(self["_NET_WM_USER_TIME"]))
            except Exception:
                pass

        @root.on()
        def ButtonPress(win, event):
            self.update_last_user_timestamp(getattr(event, 'time', None))

        @root.on()
        def KeyPress(win, event):
            self.update_last_user_timestamp(getattr(event, 'time', None) or int(time.time() * 1000))

    def update_last_user_timestamp(self, ts):
        if ts is None:
            # current server-style timestamp in milliseconds (approx)
            # X timestamps are in milliseconds since server epoch; using time.time()*1000 is acceptable for heuristics.
            ts = int(time.time() * 1000)
        if ts > self.last_user_timestamp:
            self.last_user_timestamp = ts

    def is_focusable(self, win):
        """Check if window is mapped and not override-redirect. Returns False for windows we shouldn't focus."""
        try:
            attrs = win.get_attributes()
            if attrs is None:
                return False
            if attrs.map_state != Xlib.X.IsViewable:
                return False
            if attrs.override_redirect:
                return False
            return True
        except Exception:
            return False

    def send_client_message(target_win, message_type, data, fmt=32):
        ev = Xlib.protocol.event.ClientMessage(
            window=target_win,
            client_type=self.display.intern_atom(message_type),
            data=(fmt, data)
        )
        self.display.send_event(target_win, ev, event_mask=0, propagate=False)
        self.display.flush()

    def set_active_window(self, win, timestamp=0):
        if win is None:
            root["_NET_ACTIVE_WINDOW"] = ("WINDOW", [])
            d.flush()
            return

        root["_NET_ACTIVE_WINDOW"] = win
        d.flush()

        if not is_focusable(win):
            # Some WMs might raise or set urgency; here we do nothing
            return

        # Read ICCCM WM_HINTS
        accepts_input = True
        try:
            hints = win.get_wm_hints()
        except Exception:
            pass
        else:
            if hints and getattr(hints, 'flags') & Xlib.Xutil.InputHint:
                try:
                    accepts_input = bool(hints.input)
                except Exception:
                    pass

        if "WM_TAKE_FOCUS" in win.get("WM_PROTOCOLS", []):
            # data.l[0] = timestamp (per ICCCM section "Input focus")
            # Note: after sending WM_TAKE_FOCUS the client is expected to call SetInputFocus itself
            self.send_client_message(win, "WM_TAKE_FOCUS", [timestamp or 0, 0, 0, 0, 0])
            return

        # If WM_HINTS says input == False, don't forcibly set focus (ICCCM guidance);
        # some toolkits still expect WM_TAKE_FOCUS; if they don't provide it, it's ambiguous.
        if not accepts_input:
            return

        # Otherwise set the input focus directly (XSetInputFocus)
        # Use RevertToPointerRoot to be safe; timestamp is required by ICCCM (use CurrentTime if unknown).
        t = timestamp if timestamp else Xlib.X.CurrentTime
        try:
            self.display.set_input_focus(win, Xlib.X.RevertToPointerRoot, t)
            self.display.flush()
        except Exception:
            # silently ignore; can't set focus
            pass

    def handle_net_active_window(self, event):
        """
        Handle ClientMessage _NET_ACTIVE_WINDOW sent to root.
        EWMH: data.l[0] = source indication, data.l[1] = timestamp, data.l[2] = requestor's active window
        """
        # the client message window field contains the target window to activate (per EWMH spec)
        window = event.window
        data = event.data
                 
        # python-xlib reports data as tuple (format, list/bytes)
        fmt, dvals = data.format, data.data

        src = dvals[0] if len(dvals) > 0 else 0
        ts = dvals[1] if len(dvals) > 1 else 0

        # Update last_user_timestamp heuristics: if message contains a timestamp, we may compare it
        # Accept the activation only if it's not older than last_user_timestamp (simple focus-stealing prevention)
        # EWMH allows WMs to refuse; many WMs instead set urgency.
        # If ts == 0, older clients — treat according to ALLOW_ZERO_TIMESTAMP.
        if ts == 0:
            if not self.ALLOW_ZERO_TIMESTAMP:
                # ignore / mark urgent
                return
        else:
            # if client timestamp older than last user timestamp, reject/ignore
            if (not self.ACCEPT_OLD_TIMESTAMP) and self.last_user_timestamp and ts < self.last_user_timestamp:
                # Option: set urgency hint on the window instead of focusing
                    # set urgency (ICCCM WM_HINTS: XUrgencyHint). Implementation omitted here.
                    # For demonstration we do nothing or could raise the window.
                return

        self.set_active_window(event.window, timestamp=ts)


@click.command()
@InfiniteGlass.profilable
def main(**kw):
    with InfiniteGlass.Display() as display:
        manager = Manager(display)

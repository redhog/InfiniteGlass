#include "input.h"
#include "wm.h"
#include "xapi.h"
#include "xevent.h"
#include "view.h"
#include "actions.h"

static Bool debug_positions = False;
static Bool debug_modes = False;

InputMode **input_mode_stack = NULL;
size_t input_mode_stack_len = 0;
size_t input_mode_stack_size = 10;

void input_mode_stack_configure(Window window) {
  input_mode_stack[input_mode_stack_len-1]->configure(input_mode_stack_len-1, window);
}

uint input_mode_stack_handle(XEvent event) {
  for (ssize_t idx = input_mode_stack_len - 1; idx >= 0; idx--) {
    if (input_mode_stack[idx]->handle_event(idx, event)) {
      return True;
    }
  }
  return False;
}

void push_input_mode(InputMode *mode) {
  input_mode_stack_len++;
  if (input_mode_stack_len > input_mode_stack_size || !input_mode_stack) {
    input_mode_stack = (InputMode **) realloc(input_mode_stack, sizeof(InputMode*) * input_mode_stack_size);
  }
  input_mode_stack[input_mode_stack_len-1] = mode;
  input_mode_stack[input_mode_stack_len] = NULL;
  if (mode->enter) {
    mode->enter(input_mode_stack_len-1);
  }
}

InputMode *pop_input_mode() {
  InputMode *mode = input_mode_stack[input_mode_stack_len-1];

  if (mode->exit) {
    mode->exit(input_mode_stack_len-1);
  }
  input_mode_stack[input_mode_stack_len-1] = NULL;
  input_mode_stack_len--;
  return mode;
}


void base_input_mode_enter(size_t mode) {
  for (int mod = 0; mod < 1<<8; mod++) {
    XGrabButton(display, AnyButton, Mod4Mask | mod, root, False, ButtonPressMask | ButtonReleaseMask | PointerMotionMask, GrabModeAsync, GrabModeAsync, root, XCreateFontCursor(display, XC_box_spiral));
    XGrabKey(display, AnyKey, Mod4Mask | mod, root, False, GrabModeAsync, GrabModeAsync);
  }
}
void base_input_mode_exit(size_t mode) {}
void base_input_mode_configure(size_t mode, Window window) {}
void base_input_mode_unconfigure(size_t mode, Window window) {}
uint base_input_mode_handle_event(size_t mode, XEvent event) {
  if (event.type == MotionNotify) {
    int winx, winy;
    Item *item;
    pick(event.xmotion.x_root, event.xmotion.y_root, &winx, &winy, &item);
    if (item && item_isinstance(item, &item_type_window)) {
      ItemWindow *window_item = (ItemWindow *) item;
      XSetInputFocus(display, window_item->window, RevertToNone, CurrentTime);
     
      if (debug_positions)
        printf("Point %d,%d -> %d,%d,%d\n", event.xmotion.x_root, event.xmotion.y_root, window_item->window, winx, winy); fflush(stdout);
    } else {
      if (debug_positions)
        printf("Point %d,%d -> NONE\n", event.xmotion.x_root, event.xmotion.y_root); fflush(stdout);
    }
  } else if (event.type == KeyPress && event.xkey.state & ShiftMask && event.xkey.keycode == XKeysymToKeycode(display, XK_Home)) {
    Item *item;
    int winx, winy;
    if (event.type == ButtonPress) {
      pick(event.xbutton.x_root, event.xbutton.y_root, &winx, &winy, &item);
    } else {
      Window window;
      int revert_to;
      XGetInputFocus(display, &window, &revert_to);
      if (window != root && window != overlay) {
        item = item_get_from_window(window);
      }
    }
    if (item) {
      action_zoom_screen_to_window_and_window_to_screen(&default_view, item);
    }

  } else if (event.type == KeyPress && event.xkey.keycode == XKeysymToKeycode(display, XK_Home)) {
    action_zoom_screen_home(&default_view);
  } else if (   (event.type == ButtonPress && event.xbutton.state & Mod1Mask && event.xbutton.button == 4)
             || (event.type == ButtonPress && event.xbutton.state & Mod1Mask && event.xbutton.button == 5)
             || (event.type == KeyPress && event.xkey.state & Mod1Mask && event.xkey.keycode == XKeysymToKeycode(display, XK_Next))
             || (event.type == KeyPress && event.xkey.state & Mod1Mask && event.xkey.keycode == XKeysymToKeycode(display, XK_Prior))) {
    int winx, winy;
    Item *item = NULL;

    if (event.type == ButtonPress) {
      pick(event.xbutton.x, event.xbutton.y, &winx, &winy, &item);
    } else {
      Window window;
      int revert_to;
      XGetInputFocus(display, &window, &revert_to);
      if (window != root && window != overlay) {
        item = item_get_from_window(window);
      }
    }

    if (item) {
      item_zoom_input_mode.base.first_event = event;
      item_zoom_input_mode.base.last_event = event;
      item_zoom_input_mode.item = item;
      push_input_mode((InputMode *) &item_zoom_input_mode);
      input_mode_stack_handle(event);
    }
  } else if (   (event.type == ButtonPress
                 && (   event.xbutton.button == 4
                     || event.xbutton.button == 5))
             || (event.type == KeyPress
                 && (   event.xkey.keycode == XKeysymToKeycode(display, XK_Next)
                     || event.xkey.keycode == XKeysymToKeycode(display, XK_Prior)))) {
   push_input_mode((InputMode *) &zoom_input_mode);
   input_mode_stack_handle(event);
  } else if (   (event.type == ButtonPress && event.xbutton.button == 2)
             || (event.type == ButtonPress && event.xbutton.button == 1 && event.xbutton.state & ControlMask)
             || (event.type == KeyPress && event.xkey.state & ControlMask && event.xkey.keycode == XKeysymToKeycode(display, XK_Up))
             || (event.type == KeyPress && event.xkey.state & ControlMask && event.xkey.keycode == XKeysymToKeycode(display, XK_Down))
             || (event.type == KeyPress && event.xkey.state & ControlMask && event.xkey.keycode == XKeysymToKeycode(display, XK_Left))
             || (event.type == KeyPress && event.xkey.state & ControlMask && event.xkey.keycode == XKeysymToKeycode(display, XK_Right))) {
    pan_input_mode.base.first_event = event;
    pan_input_mode.base.last_event = event;
    push_input_mode((InputMode *) &pan_input_mode);
    input_mode_stack_handle(event);
  } else if (   (event.type == ButtonPress && event.xbutton.button == 1)
             || (event.type == KeyPress && event.xkey.keycode == XKeysymToKeycode(display, XK_Up))
             || (event.type == KeyPress && event.xkey.keycode == XKeysymToKeycode(display, XK_Down))
             || (event.type == KeyPress && event.xkey.keycode == XKeysymToKeycode(display, XK_Left))
             || (event.type == KeyPress && event.xkey.keycode == XKeysymToKeycode(display, XK_Right))) {
    int winx, winy;
    Item *item = NULL;

    if (event.type == ButtonPress) {
      pick(event.xbutton.x, event.xbutton.y, &winx, &winy, &item);
    } else {
      Window window;
      int revert_to;
      XGetInputFocus(display, &window, &revert_to);
      if (window != root && window != overlay) {
        item = item_get_from_window(window);
      }
    }

    if (item) {
      item_pan_input_mode.base.first_event = event;
      item_pan_input_mode.base.last_event = event;
      item_pan_input_mode.orig_item = *item;
      item_pan_input_mode.item = item;
      push_input_mode((InputMode *) &item_pan_input_mode);
      input_mode_stack_handle(event);
    }
  }
  return 1;
};
BaseInputMode base_input_mode = {
  {
    base_input_mode_enter,
    base_input_mode_exit,
    base_input_mode_configure,
    base_input_mode_unconfigure,
    base_input_mode_handle_event
  }
};


void zoom_input_mode_enter(size_t mode) {
  if(debug_modes) printf("zoom_input_mode_enter\n"); fflush(stdout);
  XGrabPointer(display, root, False, ButtonPressMask | ButtonReleaseMask | PointerMotionMask, GrabModeAsync, GrabModeAsync, root, XCreateFontCursor(display, XC_box_spiral), CurrentTime);
  XGrabKeyboard(display, root, False, GrabModeAsync, GrabModeAsync, CurrentTime);
}
void zoom_input_mode_exit(size_t mode) {
  if(debug_modes) printf("zoom_input_mode_exit\n"); fflush(stdout);
  XUngrabPointer(display, CurrentTime);
  XUngrabKeyboard(display, CurrentTime);
}
void zoom_input_mode_configure(size_t mode, Window window) {}
void zoom_input_mode_unconfigure(size_t mode, Window window) {}
uint zoom_input_mode_handle_event(size_t mode, XEvent event) {
 //print_xevent(display, &event);
  if (event.type == KeyRelease) {
    pop_input_mode();
  } else if (   (event.type == ButtonRelease && event.xbutton.button == 4 && (event.xbutton.state & ShiftMask))
             || (event.type == KeyPress && event.xkey.keycode == XKeysymToKeycode(display, XK_Prior) && (event.xkey.state & ShiftMask))) {
    // shift up -> zoom in to window
    Item *item;
    int winx, winy;
    if (event.type == ButtonPress) {
      pick(event.xbutton.x_root, event.xbutton.y_root, &winx, &winy, &item);
    } else {
      Window window;
      int revert_to;
      XGetInputFocus(display, &window, &revert_to);
      if (window != root && window != overlay) {
        item = item_get_from_window(window);
      }
    }
    if (item) {
      action_zoom_to_window(&default_view, item);
    } else {
 
    }
  } else if (event.type == ButtonRelease && event.xbutton.button == 5
             && (event.xbutton.state & ShiftMask)) { // shift down -> zoom out to window


  } else if (event.type == KeyPress && event.xkey.keycode == XKeysymToKeycode(display, XK_Prior)) { // up -> zoom in
    action_zoom_screen_by(&default_view, 1 / 1.1);
  } else if (event.type == KeyPress && event.xkey.keycode == XKeysymToKeycode(display, XK_Next)) { // down -> zoom out
    action_zoom_screen_by(&default_view, 1.1);
  } else if (event.type == ButtonRelease && event.xbutton.button == 4) { // up -> zoom in
    action_zoom_screen_by_around(&default_view, 1 / 1.1, event.xbutton.x, event.xbutton.y);
  } else if (event.type == ButtonRelease && event.xbutton.button == 5) { // down -> zoom out
    action_zoom_screen_by_around(&default_view, 1.1, event.xbutton.x, event.xbutton.y);
  }
  return 1;
};

ZoomInputMode zoom_input_mode = {
  {
    zoom_input_mode_enter,
    zoom_input_mode_exit,
    zoom_input_mode_configure,
    zoom_input_mode_unconfigure,
    zoom_input_mode_handle_event
  }
};


void pan_input_mode_enter(size_t mode) {
  if(debug_modes) printf("pan_input_mode_enter\n"); fflush(stdout);
  PanInputMode *self = (PanInputMode *) input_mode_stack[mode];
  self->x = 0;
  self->y = 0;
  memcpy(self->screen_orig, default_view.screen, sizeof default_view.screen);
  XGrabPointer(display, root, False, ButtonPressMask | ButtonReleaseMask | PointerMotionMask, GrabModeAsync, GrabModeAsync, root, XCreateFontCursor(display, XC_box_spiral), CurrentTime);
  XGrabKeyboard(display, root, False, GrabModeAsync, GrabModeAsync, CurrentTime);
}
void pan_input_mode_exit(size_t mode) {
  if(debug_modes) printf("pan_input_mode_exit\n"); fflush(stdout);
  XUngrabPointer(display, CurrentTime);
  XUngrabKeyboard(display, CurrentTime);
}
void pan_input_mode_configure(size_t mode, Window window) {}
void pan_input_mode_unconfigure(size_t mode, Window window) {}
uint pan_input_mode_handle_event(size_t mode, XEvent event) {
  PanInputMode *self = (PanInputMode *) input_mode_stack[mode];

  if (event.type == KeyRelease || event.type == ButtonRelease) {
    pop_input_mode();
  } else if (event.type == KeyPress) {
    float spacex_orig, spacey_orig;
    float spacex, spacey;

    self->x += (event.xkey.keycode == XKeysymToKeycode(display, XK_Left)) - (event.xkey.keycode == XKeysymToKeycode(display, XK_Right));
    self->y += (event.xkey.keycode == XKeysymToKeycode(display, XK_Up)) - (event.xkey.keycode == XKeysymToKeycode(display, XK_Down));

    view_to_space(&default_view, 0, 0, &spacex_orig, &spacey_orig);
    view_to_space(&default_view, self->x, self->y, &spacex, &spacey);

    if (debug_positions)
      printf("Pan %d,%d -> %f,%f\n",
             self->x,
             self->y,
             spacex - spacex_orig,
             spacey - spacey_orig); fflush(stdout);
    
    default_view.screen[0] = self->screen_orig[0] - (spacex - spacex_orig);
    default_view.screen[1] = self->screen_orig[1] - (spacey - spacey_orig);

    draw();

  } else if (self->base.first_event.type == ButtonPress && event.type == MotionNotify) {
    float spacex_orig, spacey_orig;
    float spacex, spacey;
    
    view_to_space(&default_view, self->base.first_event.xmotion.x_root,
                 self->base.first_event.xmotion.y_root,
                 &spacex_orig, &spacey_orig);
    view_to_space(&default_view, event.xmotion.x_root,
                 event.xmotion.y_root,
                 &spacex, &spacey);

    if (debug_positions)
      printf("Pan %d,%d -> %f,%f\n",
             event.xmotion.x_root - self->base.first_event.xmotion.x_root,
             event.xmotion.y_root - self->base.first_event.xmotion.y_root,
             spacex - spacex_orig,
             spacey - spacey_orig); fflush(stdout);
    
    default_view.screen[0] = self->screen_orig[0] - (spacex - spacex_orig);
    default_view.screen[1] = self->screen_orig[1] - (spacey - spacey_orig);

    draw();

    if (debug_positions)
      printf("%f,%f[%f,%f]\n",
             default_view.screen[0],default_view.screen[1],default_view.screen[2],default_view.screen[3]);

  }
  return 1;
};

PanInputMode pan_input_mode = {
  {
    pan_input_mode_enter,
    pan_input_mode_exit,
    pan_input_mode_configure,
    pan_input_mode_unconfigure,
    pan_input_mode_handle_event
  }
};


void item_zoom_input_mode_enter(size_t mode) {
  ItemZoomInputMode *self = (ItemZoomInputMode *) input_mode_stack[mode];
  if(debug_modes) printf("item_zoom_input_mode_enter\n"); fflush(stdout);
  XGrabPointer(display, root, False, ButtonPressMask | ButtonReleaseMask | PointerMotionMask, GrabModeAsync, GrabModeAsync, root, XCreateFontCursor(display, XC_box_spiral), CurrentTime);
  XGrabKeyboard(display, root, False, GrabModeAsync, GrabModeAsync, CurrentTime);
}
void item_zoom_input_mode_exit(size_t mode) {
  if(debug_modes) printf("item_zoom_input_mode_exit\n"); fflush(stdout);
  XUngrabPointer(display, CurrentTime);
  XUngrabKeyboard(display, CurrentTime);
}
void item_zoom_input_mode_configure(size_t mode, Window window) {}
void item_zoom_input_mode_unconfigure(size_t mode, Window window) {}

uint item_zoom_input_mode_handle_event(size_t mode, XEvent event) {
  ItemZoomInputMode *self = (ItemZoomInputMode *) input_mode_stack[mode];
  
//  print_xevent(display, &event);
  if (event.type == KeyRelease) {
    pop_input_mode();
  } else if (   (event.type == ButtonRelease && event.xbutton.state & ShiftMask && event.xbutton.button == 4)
             || (event.type == KeyPress && event.xbutton.state & ShiftMask && event.xkey.keycode == XKeysymToKeycode(display, XK_Prior))) {
    action_zoom_window_to_1_to_1_to_screen(&default_view, self->item);
  } else if (   (event.type == ButtonRelease && event.xbutton.state & ShiftMask && event.xbutton.button == 5)
             || (event.type == KeyPress && event.xbutton.state & ShiftMask && event.xkey.keycode == XKeysymToKeycode(display, XK_Next))) {
    action_zoom_screen_to_1_to_1_to_window(&default_view, self->item);   
  } else if (   (event.type == ButtonRelease && event.xbutton.button == 4)
             || (event.type == KeyPress && event.xkey.keycode == XKeysymToKeycode(display, XK_Prior))) {
    action_zoom_window_by(&default_view, self->item, 0.9);
  } else if (   (event.type == ButtonRelease && event.xbutton.button == 5)
             || (event.type == KeyPress && event.xkey.keycode == XKeysymToKeycode(display, XK_Next))) {
    action_zoom_window_by(&default_view, self->item, 1.1);
  }
  return 1;
};

ItemZoomInputMode item_zoom_input_mode = {
  {
    item_zoom_input_mode_enter,
    item_zoom_input_mode_exit,
    item_zoom_input_mode_configure,
    item_zoom_input_mode_unconfigure,
    item_zoom_input_mode_handle_event
  }
};


void item_pan_input_mode_enter(size_t mode) {
  ItemPanInputMode *self = (ItemPanInputMode *) input_mode_stack[mode];
  if(debug_modes) printf("item_pan_input_mode_enter\n"); fflush(stdout);
  XGrabPointer(display, root, False, ButtonPressMask | ButtonReleaseMask | PointerMotionMask, GrabModeAsync, GrabModeAsync, root, XCreateFontCursor(display, XC_box_spiral), CurrentTime);
  XGrabKeyboard(display, root, False, GrabModeAsync, GrabModeAsync, CurrentTime);
  self->x = 0;
  self->y = 0;
}
void item_pan_input_mode_exit(size_t mode) {
  if(debug_modes) printf("item_pan_input_mode_exit\n"); fflush(stdout);
  XUngrabPointer(display, CurrentTime);
  XUngrabKeyboard(display, CurrentTime);
}
void item_pan_input_mode_configure(size_t mode, Window window) {}
void item_pan_input_mode_unconfigure(size_t mode, Window window) {}

uint item_pan_input_mode_handle_event(size_t mode, XEvent event) {
  ItemPanInputMode *self = (ItemPanInputMode *) input_mode_stack[mode];
  
//  print_xevent(display, &event);
  if (event.type == KeyRelease || event.type == ButtonRelease) {
    pop_input_mode();
  } else if (event.type == KeyPress) {
    float spacex_orig, spacey_orig;
    float spacex, spacey;

    self->x += (event.xkey.keycode == XKeysymToKeycode(display, XK_Right)) - (event.xkey.keycode == XKeysymToKeycode(display, XK_Left));
    self->y += (event.xkey.keycode == XKeysymToKeycode(display, XK_Down)) - (event.xkey.keycode == XKeysymToKeycode(display, XK_Up));

    view_to_space(&default_view, 0, 0, &spacex_orig, &spacey_orig);
    view_to_space(&default_view, self->x, self->y, &spacex, &spacey);

    self->item->coords[0] =  self->orig_item.coords[0] + (spacex - spacex_orig);
    self->item->coords[1] =  self->orig_item.coords[1] + (spacey - spacey_orig);

    if (debug_positions)
      printf("Move %d,%d -> %f,%f\n",
             event.xmotion.x_root - self->base.first_event.xmotion.x_root,
             event.xmotion.y_root - self->base.first_event.xmotion.y_root,
             spacex - spacex_orig,
             spacey - spacey_orig); fflush(stdout);
    
    item_type_base.update(self->item);
    draw();

  } else if (event.type == MotionNotify) {
   //print_xevent(display, &event);
   
    float spacex_orig, spacey_orig;
    float spacex, spacey;
    
    view_to_space(&default_view, self->base.first_event.xmotion.x_root,
                 self->base.first_event.xmotion.y_root,
                 &spacex_orig, &spacey_orig);
    view_to_space(&default_view, event.xmotion.x_root,
                 event.xmotion.y_root,
                 &spacex, &spacey);

    self->item->coords[0] =  self->orig_item.coords[0] + (spacex - spacex_orig);
    self->item->coords[1] =  self->orig_item.coords[1] + (spacey - spacey_orig);

    if (debug_positions)
      printf("Move %d,%d -> %f,%f\n",
             event.xmotion.x_root - self->base.first_event.xmotion.x_root,
             event.xmotion.y_root - self->base.first_event.xmotion.y_root,
             spacex - spacex_orig,
             spacey - spacey_orig); fflush(stdout);
    
    item_type_base.update(self->item);
    draw();
  }
  return 1;
};

ItemPanInputMode item_pan_input_mode = {
  {
    item_pan_input_mode_enter,
    item_pan_input_mode_exit,
    item_pan_input_mode_configure,
    item_pan_input_mode_unconfigure,
    item_pan_input_mode_handle_event
  }
};

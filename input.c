#include "input.h"
#include "wm.h"
#include "xapi.h"
#include "xevent.h"
#include "screen.h"

Bool debug_positions = False;

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
  print_xevent(display, &event);
  if (event.type == MotionNotify) {
    int winx, winy;
    Item *item;
    pick(event.xmotion.x_root, event.xmotion.y_root, &winx, &winy, &item);
    if (item) {

      XWindowChanges values;
      values.x = event.xmotion.x_root - winx;
      values.y = event.xmotion.y_root - winy;
      values.stack_mode = Above;
      XConfigureWindow(display, item->window, CWX | CWY | CWStackMode, &values);
      XSetInputFocus(display, item->window, RevertToNone, CurrentTime);
     
      if (debug_positions)
        printf("Point %d,%d -> %d,%d,%d\n", event.xmotion.x_root, event.xmotion.y_root, item->window, winx, winy); fflush(stdout);
    }
  } else if (event.type == KeyPress && event.xkey.keycode == XKeysymToKeycode(display, XK_Home)) {
    screen[0] = 0.;
    screen[1] = 0.;
    screen[2] = 1.;
    screen[3] = (float) overlay_attr.height / (float) overlay_attr.width;
    draw();
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
        item = item_get(window);
      }
    }

    if (item) {
      item_input_mode.base.first_event = event;
      item_input_mode.base.last_event = event;
      item_input_mode.orig_item = *item;
      item_input_mode.item = item;
      push_input_mode((InputMode *) &item_input_mode);
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
  printf("zoom_input_mode_enter\n"); fflush(stdout);
  XGrabPointer(display, root, False, ButtonPressMask | ButtonReleaseMask | PointerMotionMask, GrabModeAsync, GrabModeAsync, root, XCreateFontCursor(display, XC_box_spiral), CurrentTime);
  XGrabKeyboard(display, root, False, GrabModeAsync, GrabModeAsync, CurrentTime);
}
void zoom_input_mode_exit(size_t mode) {
  printf("zoom_input_mode_exit\n"); fflush(stdout);
  XUngrabPointer(display, CurrentTime);
  XUngrabKeyboard(display, CurrentTime);
}
void zoom_input_mode_configure(size_t mode, Window window) {}
void zoom_input_mode_unconfigure(size_t mode, Window window) {}
uint zoom_input_mode_handle_event(size_t mode, XEvent event) {
 //print_xevent(display, &event);
  if (event.type == KeyRelease) {
    pop_input_mode();
  } else if (event.type == ButtonRelease && event.xbutton.button == 4
             && (event.xbutton.state & ShiftMask)) { // shift up -> zoom in to window
    Item *item;
    int winx, winy;
    pick(event.xbutton.x_root, event.xbutton.y_root, &winx, &winy, &item);
    if (item) {
      printf("%f,%f[%f,%f] - %f,%f[%f,%f]\n",
             screen[0],screen[1],screen[2],screen[3],
             item->coords[0],item->coords[1],item->coords[2],item->coords[3]);

             
screen[0] = item->coords[0]; // / item->coords[2];
screen[1] = -(item->coords[1] - item->coords[3]);// / item->coords[3];
//      screen[2] = item->coords[2];
//      screen[2] = item->coords[2] * (float) overlay_attr.height / (float) overlay_attr.width;
      draw();
    } else {
 
    }
  } else if (event.type == ButtonRelease && event.xbutton.button == 5
             && (event.xbutton.state & ShiftMask)) { // shift down -> zoom out to window


  } else if (   (event.type == ButtonRelease && event.xbutton.button == 4)
             || (event.type == KeyPress && event.xkey.keycode == XKeysymToKeycode(display, XK_Prior))) { // up -> zoom in
    float x = ((float) (event.xbutton.x - overlay_attr.x)) / (float) overlay_attr.width;
    float y = ((float) (overlay_attr.height - (event.xbutton.y - overlay_attr.y))) / (float) overlay_attr.width;

    float centerx = screen[0] + x * screen[2];
    float centery = screen[1] + y * screen[3];
  
    screen[2] = screen[2] / 1.1;
    screen[3] = screen[3] / 1.1;

    screen[0] = centerx - screen[2] / 2.;
    screen[1] = centery - screen[3] / 2.;

    draw();
  } else if (   (event.type == ButtonRelease && event.xbutton.button == 5)
             || (event.type == KeyPress && event.xkey.keycode == XKeysymToKeycode(display, XK_Next))) { // down -> zoom out
    screen[2] = screen[2] * 1.1;
    screen[3] = screen[3] * 1.1;
    draw();
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
  printf("pan_input_mode_enter\n"); fflush(stdout);
  PanInputMode *self = (PanInputMode *) input_mode_stack[mode];
  self->x = 0;
  self->y = 0;
  memcpy(self->screen_orig, screen, sizeof screen);
  XGrabPointer(display, root, False, ButtonPressMask | ButtonReleaseMask | PointerMotionMask, GrabModeAsync, GrabModeAsync, root, XCreateFontCursor(display, XC_box_spiral), CurrentTime);
  XGrabKeyboard(display, root, False, GrabModeAsync, GrabModeAsync, CurrentTime);
}
void pan_input_mode_exit(size_t mode) {
  printf("pan_input_mode_exit\n"); fflush(stdout);
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

    screen2space(0, 0, &spacex_orig, &spacey_orig);
    screen2space(self->x, self->y, &spacex, &spacey);

    if (debug_positions)
      printf("Pan %d,%d -> %f,%f\n",
             self->x,
             self->y,
             spacex - spacex_orig,
             spacey - spacey_orig); fflush(stdout);
    
    screen[0] = self->screen_orig[0] - (spacex - spacex_orig);
    screen[1] = self->screen_orig[1] - (spacey - spacey_orig);

    draw();

  } else if (self->base.first_event.type == ButtonPress && event.type == MotionNotify) {
    float spacex_orig, spacey_orig;
    float spacex, spacey;
    
    screen2space(self->base.first_event.xmotion.x_root,
                 self->base.first_event.xmotion.y_root,
                 &spacex_orig, &spacey_orig);
    screen2space(event.xmotion.x_root,
                 event.xmotion.y_root,
                 &spacex, &spacey);

    if (debug_positions)
      printf("Pan %d,%d -> %f,%f\n",
             event.xmotion.x_root - self->base.first_event.xmotion.x_root,
             event.xmotion.y_root - self->base.first_event.xmotion.y_root,
             spacex - spacex_orig,
             spacey - spacey_orig); fflush(stdout);
    
    screen[0] = self->screen_orig[0] - (spacex - spacex_orig);
    screen[1] = self->screen_orig[1] - (spacey - spacey_orig);

    draw();
   
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


void item_input_mode_enter(size_t mode) {
  ItemInputMode *self = (ItemInputMode *) input_mode_stack[mode];
  printf("item_input_mode_enter\n"); fflush(stdout);
  XGrabPointer(display, root, False, ButtonPressMask | ButtonReleaseMask | PointerMotionMask, GrabModeAsync, GrabModeAsync, root, XCreateFontCursor(display, XC_box_spiral), CurrentTime);
  XGrabKeyboard(display, root, False, GrabModeAsync, GrabModeAsync, CurrentTime);
  self->x = 0;
  self->y = 0;
}
void item_input_mode_exit(size_t mode) {
  printf("item_input_mode_exit\n"); fflush(stdout);
  XUngrabPointer(display, CurrentTime);
  XUngrabKeyboard(display, CurrentTime);
}
void item_input_mode_configure(size_t mode, Window window) {}
void item_input_mode_unconfigure(size_t mode, Window window) {}

uint item_input_mode_handle_event(size_t mode, XEvent event) {
  ItemInputMode *self = (ItemInputMode *) input_mode_stack[mode];
  
//  print_xevent(display, &event);
  if (event.type == KeyRelease || event.type == ButtonRelease) {
    pop_input_mode();
  } else if (event.type == KeyPress) {
    float spacex_orig, spacey_orig;
    float spacex, spacey;

    self->x += (event.xkey.keycode == XKeysymToKeycode(display, XK_Right)) - (event.xkey.keycode == XKeysymToKeycode(display, XK_Left));
    self->y += (event.xkey.keycode == XKeysymToKeycode(display, XK_Down)) - (event.xkey.keycode == XKeysymToKeycode(display, XK_Up));

    screen2space(0, 0, &spacex_orig, &spacey_orig);
    screen2space(self->x, self->y, &spacex, &spacey);

    self->item->coords[0] =  self->orig_item.coords[0] + (spacex - spacex_orig);
    self->item->coords[1] =  self->orig_item.coords[1] + (spacey - spacey_orig);

    if (debug_positions)
      printf("Move %d,%d -> %f,%f\n",
             event.xmotion.x_root - self->base.first_event.xmotion.x_root,
             event.xmotion.y_root - self->base.first_event.xmotion.y_root,
             spacex - spacex_orig,
             spacey - spacey_orig); fflush(stdout);
    
    item_update_space_pos(self->item);
    draw();

  } else if (event.type == MotionNotify) {
   //print_xevent(display, &event);
   
    float spacex_orig, spacey_orig;
    float spacex, spacey;
    
    screen2space(self->base.first_event.xmotion.x_root,
                 self->base.first_event.xmotion.y_root,
                 &spacex_orig, &spacey_orig);
    screen2space(event.xmotion.x_root,
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
    
    item_update_space_pos(self->item);
    draw();
  }
  return 1;
};

ItemInputMode item_input_mode = {
  {
    item_input_mode_enter,
    item_input_mode_exit,
    item_input_mode_configure,
    item_input_mode_unconfigure,
    item_input_mode_handle_event
  }
};

#include "input.h"
#include "wm.h"
#include "xapi.h"
#include "xevent.h"
#include "screen.h"

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
  XGrabButton(display, AnyButton, ControlMask | Mod1Mask, root, False, ButtonPressMask | ButtonReleaseMask | PointerMotionMask, GrabModeAsync, GrabModeAsync, root, XCreateFontCursor(display, XC_box_spiral));
}
void base_input_mode_exit(size_t mode) {}
void base_input_mode_configure(size_t mode, Window window) {}
void base_input_mode_unconfigure(size_t mode, Window window) {}
uint base_input_mode_handle_event(size_t mode, XEvent event) {
 //print_xevent(display, &event);
  if (event.type == ButtonPress
      && event.xbutton.button == 1) {
    int winx, winy;
    Item *item;
    pick(event.xbutton.x, event.xbutton.y, &winx, &winy, &item);
    if (item) {
      item_input_mode.base.first_event = event;
      item_input_mode.base.last_event = event;
      item_input_mode.orig_item = *item;
      item_input_mode.item = item;
      push_input_mode((InputMode *) &item_input_mode);
    }
  } else if (event.type == ButtonPress
      && event.xbutton.button == 2) {
    pan_input_mode.base.first_event = event;
    pan_input_mode.base.last_event = event;
    push_input_mode((InputMode *) &pan_input_mode);
  } else if (event.type == ButtonPress
             && (   event.xbutton.button == 4
                 || event.xbutton.button == 5)) {
   push_input_mode((InputMode *) &zoom_input_mode);
/*
 } else if (event.type == ButtonRelease && event.xbutton.button == 1) { //click
    int winx, winy;
    Item *item;
    pick(event.xbutton.x, event.xbutton.y, &winx, &winy, &item);
    if (item) {
      fprintf(stderr, "Pick %d,%d -> %d,%d,%d\n", event.xbutton.x, event.xbutton.y, (int) item->window, winx, winy);
      fflush(stdout);

      XWindowChanges values;
      values.x = 0;
      values.y = 0;
      values.stack_mode = Above;
      XConfigureWindow(display, item->window, CWX | CWY | CWStackMode, &values);

      //XSetInputFocus(display, item->window, RevertToNone, CurrentTime);
      
      //overlay_set_input(False);
       //XTestFakeMotionEvent(display, -1, winx, winy, 0);
       //XTestFakeButtonEvent(display, event.xbutton.button, 1, 0);
       //overlay_set_input(True);
    } else {
      XSetInputFocus(display, root, RevertToNone, CurrentTime);
    }
*/
  }
  return 0;
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
  } else if (event.type == ButtonRelease && event.xbutton.button == 4) { // up -> zoom in
    float x = ((float) (event.xbutton.x - overlay_attr.x)) / (float) overlay_attr.width;
    float y = ((float) (overlay_attr.height - (event.xbutton.y - overlay_attr.y))) / (float) overlay_attr.width;

    float centerx = screen[0] + x * screen[2];
    float centery = screen[1] + y * screen[3];
  
    screen[2] = screen[2] / 1.1;
    screen[3] = screen[3] / 1.1;

    screen[0] = centerx - screen[2] / 2.;
    screen[1] = centery - screen[3] / 2.;

    draw();
  } else if (event.type == ButtonRelease && event.xbutton.button == 5) { // down -> zoom out
    screen[2] = screen[2] * 1.1;
    screen[3] = screen[3] * 1.1;   
    draw();
  }
  return 0;
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

  if (event.type == KeyRelease) {
    pop_input_mode();
  } else if (event.type == MotionNotify) {
    float spacex, spacey;
    
    screen2space(event.xmotion.x_root - self->base.first_event.xmotion.x_root,
                 event.xmotion.y_root - self->base.first_event.xmotion.y_root,
                 &spacex, &spacey);

    screen[0] = self->screen_orig[0] + spacex;
    screen[1] = self->screen_orig[1] + spacey;

    draw();
   
  }
  return 0;
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
  printf("item_input_mode_enter\n"); fflush(stdout);
  XGrabPointer(display, root, False, ButtonPressMask | ButtonReleaseMask | PointerMotionMask, GrabModeAsync, GrabModeAsync, root, XCreateFontCursor(display, XC_box_spiral), CurrentTime);
  XGrabKeyboard(display, root, False, GrabModeAsync, GrabModeAsync, CurrentTime);
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
  if (event.type == KeyRelease) {
    pop_input_mode();
  } else if (event.type == MotionNotify) {
   //print_xevent(display, &event);
   
    float spacex, spacey;
    
    screen2space(event.xmotion.x_root - self->base.first_event.xmotion.x_root,
                 event.xmotion.y_root - self->base.first_event.xmotion.y_root,
                 &spacex, &spacey);

    self->item->coords[0] =  self->orig_item.coords[0] + spacex;
    self->item->coords[1] =  self->orig_item.coords[1] + spacey;
/*
    printf("Motion %i,%i -> %f,%f\n",
           event.xmotion.x_root - self->base.first_event.xmotion.x_root,
           event.xmotion.y_root - self->base.first_event.xmotion.y_root,
           spacex, spacey);
    fflush(stdout);
*/
    item_update_space_pos(self->item);
    draw();
  }
  return 0;
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

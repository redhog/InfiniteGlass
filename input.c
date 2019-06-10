#include "input.h"
#include "wm.h"
#include "xapi.h"
#include "xevent.h"

InputMode **input_mode_stack = NULL;
size_t input_mode_stack_len = 0;
size_t input_mode_stack_size = 10;

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


uint base_input_mode_enter(size_t mode) {
// XGrabKey(display, AnyKey, ControlMask | Mod1Mask, root, False, GrabModeAsync, GrabModeAsync);
 for (int button = 1; button < 6; button++) {
  XGrabButton(display,
              button,
              ControlMask | Mod1Mask,
              root,
              False,
              ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
              GrabModeAsync,
              GrabModeAsync,
              None,
              XCreateFontCursor(display, XC_box_spiral));
 }
}

uint base_input_mode_handle_event(size_t mode, XEvent event) {
  print_xevent(display, &event);
/* if (event.type == KeyPress && event.xkey.state & (ControlMask | Mod1Mask) == ControlMask | Mod1Mask) {
   XGrabPointer(display, root, False, ButtonPressMask | ButtonReleaseMask | PointerMotionMask, GrabModeAsync, GrabModeAsync, root, XCreateFontCursor(display, XC_box_spiral), CurrentTime);
   } else */
  if (event.type == ButtonRelease && event.xbutton.button == 4) { // up -> zoom in
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
  } else if (event.type == ButtonRelease && event.xbutton.button == 1) { //click
    int winx, winy;
    Item *item;
    pick(event.xbutton.x, event.xbutton.y, &winx, &winy, &item);
    if (item) {
      fprintf(stderr, "Pick %d,%d -> %d,%d,%d\n", event.xbutton.x, event.xbutton.y, (int) item->window, winx, winy);
      
      XWindowChanges values;
      values.x = 0;
      values.y = 0;
      values.stack_mode = Above;
      XConfigureWindow(display, item->window, CWX | CWY | CWStackMode, &values);

      //XSetInputFocus(display, item->window, RevertToNone, CurrentTime);
      
       overlay_set_input(False);
       //XTestFakeMotionEvent(display, -1, winx, winy, 0);
       //XTestFakeButtonEvent(display, event.xbutton.button, 1, 0);
       overlay_set_input(True);
      
    } else {
      XSetInputFocus(display, root, RevertToNone, CurrentTime);
    }
  }
  return 0;
};

BaseInputMode base_input_mode = {
  {
    base_input_mode_enter,
    NULL,
    base_input_mode_handle_event
  }
};
ZoomPanInputMode zoom_pan_input_mode = {
};
ItemInputMode item_input_mode = {
};

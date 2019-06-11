#include "input.h"
#include "wm.h"
#include "xapi.h"
#include "xevent.h"

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
  print_xevent(display, &event);
  if (   ((event.type == KeyPress) && ((event.xkey.state & (ControlMask | Mod1Mask)) == (ControlMask | Mod1Mask)))
      || ((event.type == ButtonPress) && ((event.xbutton.state & (ControlMask | Mod1Mask)) == (ControlMask | Mod1Mask)))) {
    push_input_mode(&zoom_pan_input_mode);
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
      
      //overlay_set_input(False);
       //XTestFakeMotionEvent(display, -1, winx, winy, 0);
       //XTestFakeButtonEvent(display, event.xbutton.button, 1, 0);
       //overlay_set_input(True);
      
    } else {
      XSetInputFocus(display, root, RevertToNone, CurrentTime);
    }
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


void zoom_pan_input_mode_enter(size_t mode) {
  printf("zoom_pan_input_mode_enter");
  XGrabPointer(display, root, False, ButtonPressMask | ButtonReleaseMask | PointerMotionMask, GrabModeAsync, GrabModeAsync, root, XCreateFontCursor(display, XC_box_spiral), CurrentTime);
  XGrabKeyboard(display, root, False, GrabModeAsync, GrabModeAsync, CurrentTime);
}
void zoom_pan_input_mode_exit(size_t mode) {
  printf("zoom_pan_input_mode_exit");
  XUngrabPointer(display, CurrentTime);
  XUngrabKeyboard(display, CurrentTime);
}
void zoom_pan_input_mode_configure(size_t mode, Window window) {}
void zoom_pan_input_mode_unconfigure(size_t mode, Window window) {}
uint zoom_pan_input_mode_handle_event(size_t mode, XEvent event) {
  print_xevent(display, &event);
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
  } else if (event.type == ButtonRelease && event.xbutton.button == 1) { //click
    int winx, winy;
    Item *item;
    pick(event.xbutton.x, event.xbutton.y, &winx, &winy, &item);
    if (item) {
      printf("Item input\n");

      item_input_mode.x_root = event.xbutton.x_root
      item_input_mode.y_root = event.xbutton.y_root;
      item_input_mode.winx = winx;
      item_input_mode.winy = winy;
      item_input_mode.item = item;
      push_input_mode(&item_input_mode);
    }
  }
  return 0;
};

ZoomPanInputMode zoom_pan_input_mode = {
  {
    zoom_pan_input_mode_enter,
    zoom_pan_input_mode_exit,
    zoom_pan_input_mode_configure,
    zoom_pan_input_mode_unconfigure,
    zoom_pan_input_mode_handle_event
  }
};


void item_input_mode_enter(size_t mode) {
  printf("item_input_mode_enter");
  XGrabPointer(display, root, False, ButtonPressMask | ButtonReleaseMask | PointerMotionMask, GrabModeAsync, GrabModeAsync, root, XCreateFontCursor(display, XC_box_spiral), CurrentTime);
  XGrabKeyboard(display, root, False, GrabModeAsync, GrabModeAsync, CurrentTime);
}
void item_input_mode_exit(size_t mode) {
  printf("item_input_mode_exit");
  XUngrabPointer(display, CurrentTime);
  XUngrabKeyboard(display, CurrentTime);
}
void item_input_mode_configure(size_t mode, Window window) {}
void item_input_mode_unconfigure(size_t mode, Window window) {}

void mat4mul(float *mat4, float *vec4, float *outvec4) {
  for (int i = 0; i < 4; i++) {
   float res = 0.0;
    for (int j = 0; j < 4; j++) {
      res += mat4[i*4 + j];
    }
    outvec4[i] = res;
  }
}

void screen2space(float screenx, float screeny, float *spacex, float *spacey) {
  float w = (float) overlay_attr.width;
  float h = (float) overlay_attr.height;
  float screen2space[4*4] = {screen[2]/w,0,0,screen[0],
                             0,screen[3]/h,0,screen[1],
                             0,0,1,0,
                             0,0,0,1};
  float space[4] = {screenx, screeny, 0., 1.};
  float outvec[4];
  mat4mul(screen2space, space, outvec);
  *spacex = outvec[0];
  *spacey = outvec[1];
}
void space2screen(float spacex, float spacey, float *screenx, float *screeny) {
  float w = (float) overlay_attr.width;
  float h = (float) overlay_attr.height;
  float space2screen[4*4] = {w/screen[2], 0., 0., -w*screen[0]/screen[2],
                             0., h/screen[3], 0., -h*screen[1]/screen[3],
                             0., 0., 1., 0.,
                             0., 0., 0., 1.};
  float space[4] = {spacex, spacey, 0., 1.};
  float outvec[4];
  mat4mul(space2screen, space, outvec);
  *screenx = outvec[0];
  *screeny = outvec[1];
}

uint item_input_mode_handle_event(size_t mode, XEvent event) {
  ItemInputMode *self = (ItemInputMode *) input_mode_stack[mode];
  
  print_xevent(display, &event);
  if (event.type == KeyRelease) {
    pop_input_mode();
  } else if (event.type == MotionNotify) {
    float spacex, spacey;
    
    screen2space(event.xmotion.x_root - self->x_root, event.xmotion.y_root - self->y_root, &spacex, &spacey);

    item->coords[0]
    item->coords[1]

  
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

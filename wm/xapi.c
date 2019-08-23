#include "xapi.h"
#include "item_window.h"

t_glx_bind glXBindTexImageEXT = 0;
t_glx_release glXReleaseTexImageEXT = 0;

int x_default_error_handler(Display* display, XErrorEvent* e) {
  const int MAX_ERROR_TEXT_LENGTH = 1024;
  char error_text[MAX_ERROR_TEXT_LENGTH];
  XGetErrorText(display, e->error_code, error_text, sizeof(error_text));
  fprintf(stderr,
          "%s: %i - %s: request: %i, resource: %u\n",
          x_get_error_context(),
          e->error_code,
          error_text,
          e->request_code,
          (uint) e->resourceid);
  if (e->error_code != BadWindow) {
    // BadWindow usually just means the window dissappeared under our feet, so no need to get all excited and exit...
    *((char *) 0) = 0;
  }
  return 0;
}

XErrorHandler x_err_handler_stack[20];
int x_err_handler_stack_pointer = -1;

char *x_err_context_stack[20];
int x_err_context_stack_pointer = -1;

void x_push_error_handler(XErrorHandler handler) {
  x_err_handler_stack_pointer++;
  x_err_handler_stack[x_err_handler_stack_pointer] = handler;
  XSetErrorHandler(handler);
}

void x_pop_error_handler() {
  XSync(display, False);

  x_err_handler_stack_pointer--;
  XSetErrorHandler(x_err_handler_stack[x_err_handler_stack_pointer]);
}

void x_push_error_context(char *name) {
  XSync(display, False);
  x_err_context_stack_pointer++;
  x_err_context_stack[x_err_context_stack_pointer] = name;
}

char *x_get_error_context() {
  return x_err_context_stack[x_err_context_stack_pointer];
}

void x_pop_error_context() {
  XSync(display, False);
  x_err_context_stack_pointer--;
}

XErrorEvent x_try_error;
Bool x_try_has_error;

int x_try_error_handler(Display *display, XErrorEvent *e) {
  x_try_error = *e;
  x_try_has_error = True;
  return 0;
}

void x_try() {
  x_try_has_error = False;
  x_push_error_handler(&x_try_error_handler);
}

int x_catch(XErrorEvent *error) {
  Bool res;

  x_pop_error_handler();

  res = x_try_has_error;
  *error = x_try_error;

  x_try_has_error = False;

  return !res;
}

int xinit() {
  XErrorEvent error;

  display = XOpenDisplay(NULL);

  x_push_error_handler(&x_default_error_handler);
  x_push_error_context("(No context)");

  root = DefaultRootWindow(display);
  WM_PROTOCOLS = XInternAtom(display, "WM_PROTOCOLS", False);
  WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", False);
  DISPLAYSVG = XInternAtom(display, "DISPLAYSVG", False);

  IG_VIEWS = XInternAtom(display, "IG_VIEWS", False);
  IG_LAYER = XInternAtom(display, "IG_LAYER", False);
  IG_LAYER_DESKTOP = XInternAtom(display, "IG_LAYER_DESKTOP", False);
  IG_LAYER_OVERLAY = XInternAtom(display, "IG_LAYER_OVERLAY", False);
  IG_LAYER_MENU = XInternAtom(display, "IG_LAYER_MENU", False);
  IG_COORDS = XInternAtom(display, "IG_COORDS", False);
  IG_SIZE = XInternAtom(display, "IG_SIZE", False);
  IG_EXIT = XInternAtom(display, "IG_EXIT", False);
  XA_FLOAT = XInternAtom(display, "FLOAT", False);
  IG_NOTIFY_MOTION = XInternAtom(display, "IG_NOTIFY_MOTION", False);
  IG_ACTIVE_WINDOW = XInternAtom(display, "IG_ACTIVE_WINDOW", False);
  WM_STATE = XInternAtom(display, "WM_STATE", False);
  XA_MANAGER = XInternAtom(display, "MANAGER", False);    

  x_try();
  XSelectInput(display, root,
               SubstructureRedirectMask |
               SubstructureNotifyMask |
               PropertyChangeMask |
               PointerMotionMask);
  if (!x_catch(&error)) {
    fprintf(stderr, "Another window manager is already running"); fflush(stderr);
    return 0;
  }

  int event_base, error_base;
  if (!XCompositeQueryExtension(display, &event_base, &error_base)) {
    fprintf(stderr, "X server does not support the Composite extension"); fflush(stderr);
    return 0;
  }

  int major = 0, minor = 3;
  XCompositeQueryVersion(display, &major, &minor);	
  if (major == 0 && minor < 3) {
    fprintf(stderr, "X server Composite extension is too old %i.%i < 0.3)", major, minor); fflush(stderr);
    return 0;
  }

  extensions = glXQueryExtensionsString(display, 0); fflush(stderr);

  printf("Extensions: %s\n", extensions);
  if(! strstr(extensions, "GLX_EXT_texture_from_pixmap")) {
    fprintf(stderr, "GLX_EXT_texture_from_pixmap not supported!\n");
    return 0;
  }

  glXBindTexImageEXT = (t_glx_bind) glXGetProcAddress((const GLubyte *)"glXBindTexImageEXT");
  glXReleaseTexImageEXT = (t_glx_release) glXGetProcAddress((const GLubyte *)"glXReleaseTexImageEXT");

  if(!glXBindTexImageEXT || !glXReleaseTexImageEXT) {
    fprintf(stderr, "Some extension functions missing!"); fflush(stderr);
    return 0;
  }

  overlay = XCompositeGetOverlayWindow(display, root);
  XGetWindowAttributes(display, overlay, &overlay_attr);

  overlay_set_input(False);

  Cursor cursor;
  cursor=XCreateFontCursor(display,XC_left_ptr);
  XDefineCursor(display, overlay, cursor);
  XFreeCursor(display, cursor);

  XDamageQueryExtension(display, &damage_event, &damage_error);
  XShapeQueryExtension(display, &shape_event, &shape_error);

  XSync(display, False);

  fprintf(stderr, "root=%ld, overlay=%ld\n", root, overlay);

  return 1;
}

void overlay_set_input(Bool enabled) {
  XRectangle rect;
  rect.x = 0;
  rect.y = 0;
  rect.width = 0;
  rect.height = 0;
  if (enabled) {
    rect.width = overlay_attr.width;
    rect.height = overlay_attr.height;
  }
  XserverRegion region = XFixesCreateRegion(display, &rect, 1);
  XFixesSetWindowShapeRegion(display, overlay, ShapeInput, 0, 0, region);
  XFixesDestroyRegion(display, region);
}

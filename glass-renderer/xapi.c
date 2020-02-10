#include "xapi.h"
#include "item.h"
#include <sys/types.h>
#include <unistd.h>
#include "error.h"
#include "debug.h"
#include <X11/extensions/XInput2.h>

t_glx_bind glXBindTexImageEXT = 0;
t_glx_release glXReleaseTexImageEXT = 0;

void x_push_error_context(char *name) {
  XSync(display, False);
  ErrorHandler *handler = malloc(sizeof(ErrorHandler));
  handler->handler = NULL;
  handler->data = NULL;
  handler->context = name;
  error_handler_push(handler);
}

void x_pop_error_context() {
  XSync(display, False);
  ErrorHandler *handler = error_handler_pop();
  free(handler);
}

void x_try() {
  try();
}

int x_catch(XErrorEvent *error) {
  return catch(error);
}

int xinit() {
  XErrorEvent error;

  display = XOpenDisplay(NULL);

  error_init();

  root = DefaultRootWindow(display);
  IG_DEBUG = XInternAtom(display, "IG_DEBUG", False);
  IG_EXIT = XInternAtom(display, "IG_EXIT", False);
  XA_FLOAT = XInternAtom(display, "FLOAT", False);

  x_try();
  XSelectInput(display, root,
               SubstructureRedirectMask |
               SubstructureNotifyMask |
               PropertyChangeMask);
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

  DEBUG("init", "Extensions: %s\n", extensions);
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


  int ximajor = 2;
  int ximinor = 2;
  int xiret;

  xiret = XIQueryVersion(display, &ximajor, &ximinor);
  if (xiret == BadRequest) {
    printf("No XI2 support. Server supports version %d.%d only.\n", ximajor, ximinor);
    return 0;
  } else if (xiret != Success) {
    fprintf(stderr, "Internal Error! This is a bug in Xlib.\n");
    return 0;
  }
  

  XIEventMask evmasks[1];
  unsigned char mask1[(XI_LASTEVENT + 7)/8];

  memset(mask1, 0, sizeof(mask1));
  XISetMask(mask1, XI_RawMotion);

  evmasks[0].deviceid = XIAllMasterDevices;
  evmasks[0].mask_len = sizeof(mask1);
  evmasks[0].mask = mask1;

  XISelectEvents(display, root, evmasks, 1);
  XFlush(display);
  
  XSync(display, False);

  DEBUG("init", "root=%ld, overlay=%ld\n", root, overlay);

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

Atom atom_append(Display *display, Atom base, char *suffix) {
  char *strbase = XGetAtomName(display, base);
  char appended[strlen(strbase) + strlen(suffix) + 1];
  strcpy(appended, strbase);
  strcpy(appended + strlen(strbase), suffix);
  XFree(strbase);
  return XInternAtom(display, appended, False);
}

#include "xapi.h"
#include "item.h"
#include <sys/types.h>
#include <unistd.h>
#include "error.h"
#include "debug.h"
#include <X11/extensions/XInput2.h>

void x_push_error_context(XConnection *conn, char *name) {
  XSync(conn->display, False);
  ErrorHandler *handler = malloc(sizeof(ErrorHandler));
  handler->conn = conn;
  handler->handler = NULL;
  handler->data = NULL;
  handler->context = name;
  error_handler_push(handler);
}

void x_pop_error_context(XConnection *conn) {
  XSync(conn->display, False);
  ErrorHandler *handler = error_handler_pop(conn);
  free(handler);
}

void x_try(XConnection *conn) {
  try(conn);
}

int x_catch(XConnection *conn, XErrorEvent *error) {
  return catch(conn, error);
}

XContext x_conn_context = -1;

XConnection *xinit() {
  XConnection *conn = malloc(sizeof(XConnection));
 
  XErrorEvent error;

  conn->display = XOpenDisplay(NULL);

  conn->root = DefaultRootWindow(conn->display);

  if (x_conn_context == -1) x_conn_context = XUniqueContext();
  XSaveContext(conn->display, conn->root, x_conn_context, (XPointer) conn);

  error_init(conn);
  
  XA_FLOAT = XInternAtom(conn->display, "FLOAT", False);

  x_try(conn);
  XSelectInput(conn->display, conn->root,
               SubstructureRedirectMask |
               SubstructureNotifyMask |
               PropertyChangeMask);
  if (!x_catch(conn, &error)) {
    fprintf(stderr, "Another window manager is already running"); fflush(stderr);
    return 0;
  }

  int event_base, error_base;
  if (!XCompositeQueryExtension(conn->display, &event_base, &error_base)) {
    fprintf(stderr, "X server does not support the Composite extension"); fflush(stderr);
    return 0;
  }

  int major = 0, minor = 3;
  XCompositeQueryVersion(conn->display, &major, &minor);	
  if (major == 0 && minor < 3) {
    fprintf(stderr, "X server Composite extension is too old %i.%i < 0.3)", major, minor); fflush(stderr);
    return 0;
  }

  conn->extensions = glXQueryExtensionsString(conn->display, 0); fflush(stderr);

  DEBUG("init", "Extensions: %s\n", conn->extensions);
  if(! strstr(conn->extensions, "GLX_EXT_texture_from_pixmap")) {
    fprintf(stderr, "GLX_EXT_texture_from_pixmap not supported!\n");
    return 0;
  }

  conn->glXBindTexImageEXT = (t_glx_bind) glXGetProcAddress((const GLubyte *)"glXBindTexImageEXT");
  conn->glXReleaseTexImageEXT = (t_glx_release) glXGetProcAddress((const GLubyte *)"glXReleaseTexImageEXT");

  if(!conn->glXBindTexImageEXT || !conn->glXReleaseTexImageEXT) {
    fprintf(stderr, "Some extension functions missing!"); fflush(stderr);
    return 0;
  }

  conn->overlay = XCompositeGetOverlayWindow(conn->display, conn->root);
  XGetWindowAttributes(conn->display, conn->overlay, &conn->overlay_attr);

  overlay_set_input(conn, False);

  Cursor cursor;
  cursor=XCreateFontCursor(conn->display,XC_left_ptr);
  XDefineCursor(conn->display, conn->overlay, cursor);
  XFreeCursor(conn->display, cursor);

  XDamageQueryExtension(conn->display, &conn->damage_event, &conn->damage_error);
  XShapeQueryExtension(conn->display, &conn->shape_event, &conn->shape_error);


  int ximajor = 2;
  int ximinor = 2;
  int xiret;

  xiret = XIQueryVersion(conn->display, &ximajor, &ximinor);
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

  XISelectEvents(conn->display, conn->root, evmasks, 1);
  XFlush(conn->display);
  
  XSync(conn->display, False);

  DEBUG("init", "root=%ld, overlay=%ld\n", conn->root, conn->overlay);

  return conn;
}

void overlay_set_input(XConnection *conn, Bool enabled) {
  XRectangle rect;
  rect.x = 0;
  rect.y = 0;
  rect.width = 0;
  rect.height = 0;
  if (enabled) {
    rect.width = conn->overlay_attr.width;
    rect.height = conn->overlay_attr.height;
  }
  XserverRegion region = XFixesCreateRegion(conn->display, &rect, 1);
  XFixesSetWindowShapeRegion(conn->display, conn->overlay, ShapeInput, 0, 0, region);
  XFixesDestroyRegion(conn->display, region);
}

Atom atom_append(Display *display, Atom base, char *suffix) {
  char *strbase = XGetAtomName(display, base);
  char appended[strlen(strbase) + strlen(suffix) + 1];
  strcpy(appended, strbase);
  strcpy(appended + strlen(strbase), suffix);
  XFree(strbase);
  return XInternAtom(display, appended, False);
}

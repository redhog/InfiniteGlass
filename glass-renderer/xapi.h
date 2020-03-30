#ifndef XAPI
#define XAPI

#define GL_GLEXT_PROTOTYPES

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/XTest.h>
#include <X11/extensions/Xfixes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "list.h"

extern XContext x_conn_context;

typedef void (*t_glx_bind)(Display *, GLXDrawable, int , const int *);
typedef void (*t_glx_release)(Display *, GLXDrawable, int);

typedef struct {
  Display* display;
  Window root;
  Window overlay;
  XWindowAttributes overlay_attr;
  const char *extensions;
  int damage_event, damage_error;
  int shape_event, shape_error;
  t_glx_bind glXBindTexImageEXT;
  t_glx_release glXReleaseTexImageEXT;
  List *error_handlers;
} XConnection;

Atom XA_FLOAT;

extern void x_push_error_handler(XConnection *conn, XErrorHandler handler);
extern void x_pop_error_handler(XConnection *conn);

extern void x_push_error_context(XConnection *conn, char *name);
extern char *x_get_error_context(XConnection *conn);
extern void x_pop_error_context(XConnection *conn);

extern void x_try(XConnection *conn);
extern int x_catch(XConnection *conn, XErrorEvent *error);

extern XConnection *xinit_basic();
extern XConnection *xinit();
extern void overlay_set_input(XConnection *conn, Bool enabled);

extern Atom atom_append(Display *display, Atom base, char *suffix);

// Atoms live as long as display lives, so we can cache them at any call point.
// This way we don't have to make global variables for them and
// initialize them somewhere central.
#define ATOM(conn, name) ({ \
  static Atom res = None; \
  if (res == None) res = XInternAtom(conn->display, name, False); \
  res; \
})


#endif

#ifndef XAPI
#define XAPI

#include <xcb/xcbext.h>
#include <xcb/bigreq.h>
#include <xcb/damage.h>
#include <xcb/dri2.h>
#include <xcb/dri3.h>
#include <xcb/glx.h>
#include <xcb/present.h>
#include <xcb/randr.h>
#include <xcb/render.h>
#include <xcb/shape.h>
#include <xcb/shm.h>
#include <xcb/sync.h>
#include <xcb/xcbext.h>
#include <xcb/xcb.h>
#include <xcb/xc_misc.h>
#include <xcb/xfixes.h>
#include <xcb/xproto.h>
#include <xcb/xcb_icccm.h>
#include <X11/Xlib-xcb.h>
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
#include "glapi.h"

Atom XA_FLOAT;


Display* display;
xcb_connection_t *xcb_display;
Window root;
Window overlay;
XWindowAttributes overlay_attr;
int nxextensions;
char **xextensions;
const char *extensions;
typedef void (*t_glx_bind)(Display *, GLXDrawable, int , const int *);
typedef void (*t_glx_release)(Display *, GLXDrawable, int);
t_glx_bind glXBindTexImageEXT;
t_glx_release glXReleaseTexImageEXT;

int damage_event, damage_error;
int shape_event, shape_error;

Bool on_xephyr;

extern void x_push_error_handler(XErrorHandler handler);
extern void x_pop_error_handler();

extern void x_push_error_context(char *name);
extern char *x_get_error_context();
extern void x_pop_error_context();

extern void x_try();
extern int x_catch(XErrorEvent *error);

extern int xinit();
extern void overlay_set_input(Bool enabled);

extern Atom atom_append(Display *display, Atom base, char *suffix);

// Atoms live as long as display lives, so we can cache them at any call point.
// This way we don't have to make global variables for them and
// initialize them somewhere central.
#define ATOM(name) ({ \
  static Atom res = None; \
  if (res == None) res = XInternAtom(display, name, False); \
  res; \
})


#endif

#ifndef XAPI
#define XAPI

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/XTest.h>
#include <X11/extensions/Xfixes.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

Atom WM_PROTOCOLS;
Atom WM_DELETE_WINDOW;

Display* display;
Window root;
Window overlay;
XWindowAttributes overlay_attr;
const char *extensions;
typedef void (*t_glx_bind)(Display *, GLXDrawable, int , const int *);
typedef void (*t_glx_release)(Display *, GLXDrawable, int);
t_glx_bind glXBindTexImageEXT;
t_glx_release glXReleaseTexImageEXT;

int damage_event, damage_error;
int shape_event, shape_error;

extern void x_push_error_handler(XErrorHandler handler);
extern void x_pop_error_handler();

extern void x_push_error_context(char *name);
extern char *x_get_error_context();
extern void x_pop_error_context();

extern void x_try();
extern int x_catch(XErrorEvent *error);

extern int xinit();
extern void overlay_set_input(Bool enabled);

#endif

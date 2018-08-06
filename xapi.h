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
Bool wm_detected;
const char *extensions;
typedef void (*t_glx_bind)(Display *, GLXDrawable, int , const int *);
typedef void (*t_glx_release)(Display *, GLXDrawable, int);
t_glx_bind glXBindTexImageEXT;
t_glx_release glXReleaseTexImageEXT;

int damage_event, damage_error;
int shape_event, shape_error;

extern int xinit();

#endif

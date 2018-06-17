#include <X11/Xutil.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xrender.h>
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
Bool wm_detected;
const char *extensions;
typedef void (*t_glx_bind)(Display *, GLXDrawable, int , const int *);
typedef void (*t_glx_release)(Display *, GLXDrawable, int);
t_glx_bind glXBindTexImageEXT;
t_glx_release glXReleaseTexImageEXT;

const int *pixmap_attribs;

extern int xinit();

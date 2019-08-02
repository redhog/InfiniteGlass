#include <X11/Xatom.h>
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
#include <stdio.h>

Display* display;
Window win;

Atom DISPLAYSVG;
Atom IG_LAYER;
Atom IG_LAYER_DESKTOP;
Atom IG_LAYER_OVERLAY;
Atom IG_X;
Atom IG_Y;
Atom IG_W;
Atom IG_H;

void readfile(FILE *file, char **buf, size_t *buf_size) {
  size_t readsize;

  *buf_size = 0;
  *buf = NULL;
  
  while (True) {
    *buf = realloc(*buf, *buf_size + 32);
    readsize = fread(*buf + *buf_size, 1, 32, file);
    *buf_size += readsize;
    if (readsize == 0) return;
  }
}

int main() {
  char *buf;
  size_t buf_size;
  display = XOpenDisplay(NULL);

  DISPLAYSVG = XInternAtom(display, "DISPLAYSVG", False);
  IG_LAYER = XInternAtom(display, "IG_LAYER", False);
  IG_LAYER_DESKTOP = XInternAtom(display, "IG_LAYER_DESKTOP", False);
  IG_LAYER_OVERLAY = XInternAtom(display, "IG_LAYER_OVERLAY", False);
  IG_X = XInternAtom(display, "IG_X", False);
  IG_Y = XInternAtom(display, "IG_Y", False);
  IG_W = XInternAtom(display, "IG_W", False);
  IG_H = XInternAtom(display, "IG_H", False);
  
  FILE *f = fopen("fontawesome-free-5.9.0-desktop/svgs/regular/map.svg", "r");
  if (!f) {
    fprintf(stderr, "Unable to read svg file\n");
    exit(1);
  }
  readfile(f, &buf, &buf_size);
  fclose(f);

  printf("PROPERTY\n%s\n%d\nEND\n", buf, buf_size);
  
  win = XCreateWindow(display, DefaultRootWindow(display), 0, 0, 100, 100, 0, DefaultDepth(display, 0), InputOutput, CopyFromParent, 0, NULL);
  XChangeProperty(display, win, DISPLAYSVG, XA_STRING, 8, PropModeReplace, buf, buf_size);
  free(buf);

  XChangeProperty(display, win, IG_LAYER, XA_ATOM, 32, PropModeReplace, &IG_LAYER_OVERLAY, 1);
  
  XMapWindow(display, win);

  for (;;) {
    XEvent e;
    XSync(display, False);
    XNextEvent(display, &e);
  }
}

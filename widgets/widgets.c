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

int main(int argc, char *argv[]) {
  char *buf;
  size_t buf_size;
  Atom action;
  
  if (argc < 3) {
    fprintf(stderr, "Usage: widgets path/to/file.svg IG_ACTION_NAME\n");
    exit(1);
  }
  
  display = XOpenDisplay(NULL);

  Atom DISPLAYSVG = XInternAtom(display, "DISPLAYSVG", False);
  Atom IG_LAYER = XInternAtom(display, "IG_LAYER", False);
  Atom IG_LAYER_OVERLAY = XInternAtom(display, "IG_LAYER_OVERLAY", False);

  action = XInternAtom(display, argv[2], False);
  XEvent ev = {0};
  ev.xclient.type = ClientMessage;
  ev.xclient.window = DefaultRootWindow(display);
  ev.xclient.message_type = action;
  ev.xclient.format = 32;
  for (int argi = 3; argi < argc; argi++) {
    if (argv[argi][0] - '0' >= 0 && argv[argi][0] - '0' < 10) {
      if (strstr(argv[argi], ".") == NULL) {
        ev.xclient.data.l[argi-3] = atoi(argv[argi]);
      } else {
       *(float *) &ev.xclient.data.l[argi-3] = atof(argv[argi]);
      }
    } else {
      ev.xclient.data.l[argi-3] = XInternAtom(display, argv[argi], False);
    }
  }

  FILE *f = fopen(argv[1], "r");
  if (!f) {
    fprintf(stderr, "Unable to read svg file\n");
    exit(1);
  }
  readfile(f, &buf, &buf_size);
  fclose(f);
  
  win = XCreateWindow(display, DefaultRootWindow(display), 0, 0, 100, 100, 0, DefaultDepth(display, 0), InputOutput, CopyFromParent, 0, NULL);
  XChangeProperty(display, win, DISPLAYSVG, XA_STRING, 8, PropModeReplace, buf, buf_size);
  free(buf);

  XChangeProperty(display, win, IG_LAYER, XA_ATOM, 32, PropModeReplace, &IG_LAYER_OVERLAY, 1);

  XSelectInput(display, win,
               KeyPressMask |
               KeyReleaseMask |
               ButtonPressMask |
               ButtonReleaseMask |
               PointerMotionMask);

  XMapWindow(display, win);

  for (;;) {
    XEvent e;
    XSync(display, False);
    XNextEvent(display, &e);

    if (e.type == ButtonPress) {
      printf("Sending %s\n", argv[2]);
      
      XSendEvent(display, DefaultRootWindow(display), False, SubstructureNotifyMask|SubstructureRedirectMask, &ev);
      XFlush(display);
      XSync(display, False);
    }
  }
}

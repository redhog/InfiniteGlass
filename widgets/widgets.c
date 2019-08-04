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

Display* display;
Window win;
Atom XA_FLOAT;

typedef struct {
  char *unparsed;
  Atom name;
  Atom type;
  union {
    float f;
    long l;
    Atom a;
    char *s;
  } data; 
} Item;

Item *parse_item(char *value) {
  char *equalsign;
  Item *item = malloc(sizeof(Item));
  item->unparsed = value;
  item->name = None;
  item->type = None;

  if ((equalsign = strstr(value, "=")) != NULL) {
    char strname[equalsign - value + 1];
    strncpy(strname, value, equalsign - value);
    strname[equalsign - value] = 0;
    value = equalsign + 1;
    item->name = XInternAtom(display, strname, False);
  }
  if (value[0] == '@') {
    item->type = XA_STRING;
    size_t readsize;
    FILE *f = fopen(value+1, "r");
    if (!f) {
      fprintf(stderr, "Unable to read file: %s\n", value+1);
      exit(1);
    }
    readfile(f, &item->data.s, &readsize);
    item->data.s[readsize] = 0;
    fclose(f);
  } else if (value[0] - '0' >= 0 && value[0] - '0' < 10) {
    if (strstr(value, ".") == NULL) {
      item->type = XA_INTEGER;
      item->data.l = atoi(value);
    } else {
      item->type = XA_FLOAT;
     item->data.f = atof(value);
    }
  } else {
    item->type = XA_ATOM;
    item->data.a = XInternAtom(display, value, False);
  }
  return item;
}

void free_item(Item *item) {
  if (item->type == XA_STRING) {
    free(item->data.s);
  }
  free(item);
}

int main(int argc, char *argv[]) {
  Item *items[argc-1];
 
  display = XOpenDisplay(NULL);
  XA_FLOAT = XInternAtom(display, "FLOAT", False);

  if (argc < 2) {
    fprintf(stderr, "Usage: widgets ATOMNAME=value ATOMNAME=value... ACTION_ATOMNAME ACTION_PARAM...\n");
    exit(1);
  }

  for (int argi = 1; argi < argc; argi++) {
    items[argi-1] = parse_item(argv[argi]);
  }

  win = XCreateWindow(display, DefaultRootWindow(display), 0, 0, 100, 100, 0, DefaultDepth(display, 0), InputOutput, CopyFromParent, 0, NULL);
  
  XEvent ev = {0};
  ev.xclient.type = ClientMessage;
  ev.xclient.window = DefaultRootWindow(display);
  ev.xclient.format = 32;

  int evargi = 0;
  for (int argi = 0; argi < argc-1; argi++) {
    size_t len = 1;
    size_t format = 32;
    void *data = &items[argi]->data;
    if (items[argi]->type == XA_STRING) { format = 8; len = strlen(items[argi]->data.s) + 1; data = items[argi]->data.s; }

    if (items[argi]->name != None) {
      XChangeProperty(display, win, items[argi]->name, items[argi]->type, format, PropModeReplace, data, len);
    } else {
      if (evargi > 5) {
        fprintf(stderr, "Only up to 6 action parameters can be sent: %s\n", items[argi]->unparsed);
        exit(1);
      }
      if (items[argi]->type == XA_STRING) {
       fprintf(stderr, "Strings (files) can not be sent as action parameters: %s\n", items[argi]->unparsed);
        exit(1);
      }
      if (evargi == 0) {
        if (items[argi]->type != XA_ATOM) {
          fprintf(stderr, "First action parameter must be of type ATOM. Atoms are simply specified by name: %s\n", items[argi]->unparsed);
          exit(1);
        }
        ev.xclient.message_type = items[argi]->data.a;
      } else {
        memcpy(&ev.xclient.data.l[evargi-1],
               data,
               sizeof(long));
      }
      evargi++;
    }
  }
  
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
      printf("Sending action.\n");
      
      XSendEvent(display, DefaultRootWindow(display), False, SubstructureNotifyMask|SubstructureRedirectMask, &ev);
      XFlush(display);
      XSync(display, False);
    }
  }
}

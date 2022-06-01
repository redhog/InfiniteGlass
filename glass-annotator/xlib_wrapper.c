#define _GNU_SOURCE

#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <X11/Xlib.h>

extern char **environ;
static char **props;
static size_t propslen;
static char *args;
static size_t argslen;

Window (*XCreateWindow_orig)(Display *display, Window parent, int x, int y, unsigned int width, unsigned int height, unsigned int border_width,
                             int depth, unsigned int class, Visual *visual, unsigned long valuemask, XSetWindowAttributes *attributes) = NULL;
pid_t (*fork_orig)(void);

int xlib_wrapper_start(int argc, char **argv, char **env) {
  XCreateWindow_orig = dlsym(RTLD_NEXT, "XCreateWindow");
  fork_orig = dlsym(RTLD_NEXT, "fork");
  
  propslen = 0;
  for (char **e = environ; *e; e++) {
    if (strncmp("IG_ANNOTATE_", *e, strlen("IG_ANNOTATE_")) == 0) {
      propslen++;
    }
  }
  props = malloc(sizeof(char *) * (propslen + 1));
  char **p = props;
  for (char **e = environ; *e; e++) {
   if (strncmp("IG_ANNOTATE_", *e, strlen("IG_ANNOTATE_")) == 0) {
      *p = malloc(strlen(*e) - strlen("IG_ANNOTATE_") + 1);
      strcpy(*p, *e + strlen("IG_ANNOTATE_"));
      char *sep = strchr(*p, '=');
      if (sep) *sep = '\0';
      p++;
    }
  }
  *p = NULL;

  argslen = 0;
  argslen += strlen("glass-action") + 1;
  argslen += strlen("run") + 1;
  for (char **p = props; *p; p++) {
    int namelen = strlen(*p);
    char * value = *p + namelen + 1;
    int valuelen = strlen(value);
    argslen += 3 + namelen + 1 + valuelen + 1;
  }
  for (char **a = argv; *a; a++) {
    argslen += strlen(*a) + 1;
  }
  
  args = malloc(argslen);
  char *b = args;
  strcpy(b, "glass-action");
  b += strlen("glass-action") + 1;
  strcpy(b, "run");
  b += strlen("run") + 1;
  for (char **p = props; *p; p++) {
    int namelen = strlen(*p);
    char *value = *p + namelen + 1;
    int valuelen = strlen(value);

    strcpy(b, "-s");
    b += 3;
    strcpy(b, *p);
    b+= namelen;
    *b = '=';
    b++;
    strcpy(b, value);
    b += valuelen + 1;
  }
  
  for (char **a = argv; *a; a++) {
    strcpy(b, *a);
    b += strlen(*a) + 1;
  }
  
  return 0;
}
__attribute__((section(".init_array"))) void *xlib_wrapper_start_constructor = &xlib_wrapper_start;

Window XCreateWindow(Display *display, Window parent, int x, int y, unsigned int width, unsigned int height, unsigned int border_width,
                     int depth, unsigned int class, Visual *visual, unsigned long valuemask, XSetWindowAttributes *attributes) {
  Window window;
 

  window = XCreateWindow_orig(display, parent, x, y, width, height, border_width,
                              depth, class, visual, valuemask, attributes);

  Atom prop_type = XInternAtom(display, "STRING", False);
  for (char **p = props; *p; p++) {
    char *value = *p + strlen(*p) + 1;
    int valuelen = strlen(value);
    if (strcmp(value, "__WM_COMMAND__") == 0) {
      value = args;
      valuelen = argslen;
    }
    Atom prop_name = XInternAtom(display, *p, False);
    XChangeProperty(display, window, prop_name, prop_type, 8, PropModeReplace, (const unsigned char *) value, valuelen);
  }
  
  return window;
}

pid_t fork(void) {
  pid_t res = fork_orig();
  if (res == 0) {
    char **e_out = environ;
    for (char **e_in = environ; *e_in; e_in++) {
      *e_out = *e_in;
      if (strncmp("IG_ANNOTATE_", *e_in, strlen("IG_ANNOTATE_")) != 0) {
        e_out++;
      }
    }
    *e_out = NULL;
  }
  return res;
}

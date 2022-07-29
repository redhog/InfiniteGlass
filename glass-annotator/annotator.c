#define _GNU_SOURCE

#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <sys/random.h>

extern char **environ;
static char **app_props;
static size_t app_propslen;
static char **group_props;
static size_t group_propslen;
static char *args;
static size_t argslen;

static char app_id[17];

Window (*XCreateWindow_orig)(Display *display, Window parent, int x, int y, unsigned int width, unsigned int height, unsigned int border_width,
                             int depth, unsigned int class, Visual *visual, unsigned long valuemask, XSetWindowAttributes *attributes) = NULL;

#define ATOM(name) ({ \
  static Atom res = None; \
  if (res == None) res = XInternAtom(display, name, False); \
  res; \
})

#define ISPREFIX(prefix, s) (strncmp(prefix, s, sizeof(prefix) - 1) == 0)


static void generate_app_id() {
  unsigned int seed;
  getrandom((void *) &seed, sizeof(seed), 0);
  srandom(seed);
  const char charset[] = "0123456789ABCDEF";
  for (size_t n = 0; n < 16; n++) {
    int key = random() % (int) (sizeof(charset) - 1);
    app_id[n] = charset[key];
  }
  app_id[16] = '\0';
}

void parse_environ() {
  app_propslen = 0;
  group_propslen = 0;
  for (char **e = environ; *e; e++) {
    if (ISPREFIX("IG_APP_ID=", *e)) {
    } else if (ISPREFIX("IG_APP_", *e)) {
      app_propslen++;
    } else if (ISPREFIX("IG_GROUP_", *e)) {
      group_propslen++;
    }
  }
  app_props = malloc(sizeof(char *) * (app_propslen + 1));
  group_props = malloc(sizeof(char *) * (group_propslen + 1));
  char **ap = app_props;
  char **gp = group_props;
  for (char **e = environ; *e; e++) {
    if (ISPREFIX("IG_APP_ID=", *e)) {
      strncpy(app_id, *e + sizeof("IG_APP_ID=") - 1, sizeof(app_id) - 1);
    } else if (ISPREFIX("IG_APP_", *e)) {
      *ap = malloc(strlen(*e) - sizeof("IG_APP_"));
      strcpy(*ap, *e + sizeof("IG_APP_") - 1);
      char *sep = strchr(*ap, '=');
      if (sep) *sep = '\0';
      ap++;
    } else if (ISPREFIX("IG_GROUP_", *e)) {
      *gp = malloc(strlen(*e) - sizeof("IG_GROUP_"));
      strcpy(*gp, *e + sizeof("IG_GROUP_") - 1);
      char *sep = strchr(*gp, '=');
      if (sep) *sep = '\0';
      gp++;
    }
  }
  *ap = NULL;
  *gp = NULL;
}

void parse_argv(char **argv) {
  argslen = 0;
  for (char **a = argv; *a; a++) {
    argslen += strlen(*a) + 1;
  }
  
  args = malloc(argslen);
  char *b = args;
  for (char **a = argv; *a; a++) {
    strcpy(b, *a);
    b += strlen(*a) + 1;
  }
}


void apply_prop(Display *display, Window window, char *prop) {
  char *name = prop;
  char *value = prop + strlen(name) + 1;
  int valuelen = strlen(value);
  Atom prop_name = XInternAtom(display, name, False);
  XChangeProperty(display, window, prop_name, XA_STRING, 8, PropModeReplace, (const unsigned char *) value, valuelen);
}

void apply_props(Display *display, Window window, char **props, Atom group) {
  int count = 0;
  for (char **p = props; *p; p++) {
    apply_prop(display, window, *p);
    count++;
  }
  Atom atoms[count];
  char **p;
  Atom *a;
  for (p = props, a = atoms; *p; p++, a++) {
    *a = XInternAtom(display, *p, False);
  }
  XChangeProperty(display, window, group, XA_ATOM, 32, PropModeReplace, (const unsigned char *) atoms, count);
}

void apply_window(Display *display, Window window) {
  apply_props(display, window, group_props, ATOM("IG_GROUP"));
  apply_props(display, window, app_props, ATOM("IG_APP"));

  XChangeProperty(display, window, ATOM("IG_APP_ID"),
                  XA_STRING, 8, PropModeReplace, (const unsigned char *) app_id, sizeof(app_id) - 1);
  XChangeProperty(display, window, XA_WM_COMMAND,
                  XA_STRING, 8, PropModeReplace, (const unsigned char *) args, argslen);
}


void filter_environ(void) {
  char **e_out = environ;
  for (char **e_in = environ; *e_in; e_in++) {
    *e_out = *e_in;
    if (!ISPREFIX("IG_APP_", *e_in)) {
      e_out++;
    }
  }
  *e_out = NULL;
}

/* Library overrides and entry points */

int xlib_wrapper_start(int argc, char **argv, char **env) {
  XCreateWindow_orig = dlsym(RTLD_NEXT, "XCreateWindow");

  generate_app_id();
  parse_environ();
  parse_argv(argv);
  filter_environ();
  
  return 0;
}
__attribute__((section(".init_array"))) void *xlib_wrapper_start_constructor = &xlib_wrapper_start;


Window XCreateWindow(Display *display, Window parent, int x, int y, unsigned int width, unsigned int height, unsigned int border_width,
                     int depth, unsigned int class, Visual *visual, unsigned long valuemask, XSetWindowAttributes *attributes) {
  Window window;

  window = XCreateWindow_orig(display, parent, x, y, width, height, border_width,
                              depth, class, visual, valuemask, attributes);
  apply_window(display, window);
  return window;
}

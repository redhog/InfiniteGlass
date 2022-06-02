#define _GNU_SOURCE

#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <sys/random.h>

extern char **environ;
static char **app_props;
static size_t app_propslen;
static char **group_props;
static size_t group_propslen;
static char *args;
static size_t argslen;

static char app_id[17];
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

Window (*XCreateWindow_orig)(Display *display, Window parent, int x, int y, unsigned int width, unsigned int height, unsigned int border_width,
                             int depth, unsigned int class, Visual *visual, unsigned long valuemask, XSetWindowAttributes *attributes) = NULL;
pid_t (*fork_orig)(void);

static Atom XA_STRING = None;

void set_props() {
  app_propslen = 0;
  group_propslen = 0;
  for (char **e = environ; *e; e++) {
    if (strncmp("IG_APP_", *e, strlen("IG_APP_")) == 0) {
      app_propslen++;
    } else if (strncmp("IG_GROUP_", *e, strlen("IG_GROUP_")) == 0) {
      group_propslen++;
    }
  }
  app_props = malloc(sizeof(char *) * (app_propslen + 1));
  group_props = malloc(sizeof(char *) * (group_propslen + 1));
  char **ap = app_props;
  char **gp = group_props;
  for (char **e = environ; *e; e++) {
    if (strncmp("IG_APP_", *e, strlen("IG_APP_")) == 0) {
      *ap = malloc(strlen(*e) - strlen("IG_APP_") + 1);
      strcpy(*ap, *e + strlen("IG_APP_"));
      char *sep = strchr(*ap, '=');
      if (sep) *sep = '\0';
      ap++;
    } else if (strncmp("IG_GROUP_", *e, strlen("IG_GROUP_")) == 0) {
      *gp = malloc(strlen(*e) - strlen("IG_GROUP_") + 1);
      strcpy(*gp, *e + strlen("IG_GROUP_"));
      char *sep = strchr(*gp, '=');
      if (sep) *sep = '\0';
      gp++;
    } else if (strncmp("IG_APPID=", *e, strlen("IG_APPID=")) == 0) {
      strncpy(app_id, *e + strlen("IG_APPID="), sizeof(app_id) - 1); 
    }
  }
  *ap = NULL;
  *gp = NULL;
}

char *copy_prop_to_arg(char *out, char *prop, char *flag) {
  char *name = prop;
  int namelen = strlen(name);
  char *value = prop + namelen + 1;
  int valuelen = strlen(value);

  strcpy(out, flag);
  out += strlen(flag) + 1;
  strcpy(out, name);
  out+= namelen;
  *out = '=';
  out++;
  strcpy(out, value);
  out += valuelen + 1;
  return out;
}

void set_args(char **argv) {
  argslen = 0;
  argslen += sizeof("glass-action");
  argslen += sizeof("run");
  argslen += sizeof("-i");
  argslen += sizeof(app_id);

  for (char **p = group_props; *p; p++) {
    int namelen = strlen(*p);
    char * value = *p + namelen + 1;
    int valuelen = strlen(value);
    argslen += 3 + namelen + 1 + valuelen + 1;
  }
  for (char **p = app_props; *p; p++) {
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
  b += sizeof("glass-action");
  strcpy(b, "run");
  b += sizeof("run");
  strcpy(b, "-i");
  b += 3;
  strcpy(b, app_id);
  b += sizeof(app_id);
  for (char **p = group_props; *p; p++) {
    b = copy_prop_to_arg(b, *p, "-g");
  }
  for (char **p = app_props; *p; p++) {
    b = copy_prop_to_arg(b, *p, "-a");
  }
  
  for (char **a = argv; *a; a++) {
    strcpy(b, *a);
    b += strlen(*a) + 1;
  }
}

int xlib_wrapper_start(int argc, char **argv, char **env) {
  XCreateWindow_orig = dlsym(RTLD_NEXT, "XCreateWindow");
  fork_orig = dlsym(RTLD_NEXT, "fork");

  generate_app_id();
  set_props();
  set_args(argv);
  
  return 0;
}
__attribute__((section(".init_array"))) void *xlib_wrapper_start_constructor = &xlib_wrapper_start;

void apply_prop(Display *display, Window window, char *prop) {
  char *name = prop;
  char *value = prop + strlen(name) + 1;
  int valuelen = strlen(value);
  if (strcmp(value, "__WM_COMMAND__") == 0) {
    value = args;
    valuelen = argslen;
  } else if (strcmp(value, "__APPID__") == 0) {
    value = app_id;
    valuelen = sizeof(app_id) - 1;
  }
  Atom prop_name = XInternAtom(display, name, False);
  XChangeProperty(display, window, prop_name, XA_STRING, 8, PropModeReplace, (const unsigned char *) value, valuelen);
}

Window XCreateWindow(Display *display, Window parent, int x, int y, unsigned int width, unsigned int height, unsigned int border_width,
                     int depth, unsigned int class, Visual *visual, unsigned long valuemask, XSetWindowAttributes *attributes) {
  Window window;

  window = XCreateWindow_orig(display, parent, x, y, width, height, border_width,
                              depth, class, visual, valuemask, attributes);

  if (XA_STRING == None) XA_STRING = XInternAtom(display, "STRING", False);
  for (char **p = group_props; *p; p++) {
    apply_prop(display, window, *p);
  }
  for (char **p = app_props; *p; p++) {
    apply_prop(display, window, *p);
  }
  
  return window;
}

pid_t fork(void) {
  pid_t res = fork_orig();
  if (res == 0) {
    char **e_out = environ;
    for (char **e_in = environ; *e_in; e_in++) {
      *e_out = *e_in;
      if (   (strncmp("IG_APP_", *e_in, strlen("IG_APP_")) != 0)
          && (strcmp("IG_APPID", *e_in) != 0)) {
        e_out++;
      }
    }
    *e_out = NULL;
  }
  return res;
}

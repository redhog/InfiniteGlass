#define _GNU_SOURCE

#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <sys/random.h>

#define TYPE_APP 0
#define TYPE_GROUP 1

extern char **environ;
static char **props;
static size_t propslen;
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

void set_props() {
  propslen = 0;
  for (char **e = environ; *e; e++) {
   if (   (strncmp("IG_APP_", *e, strlen("IG_APP_")) == 0)
       || (strncmp("IG_GROUP_", *e, strlen("IG_GROUP_")) == 0)) {
      propslen++;
    }
  }
  props = malloc(sizeof(char *) * (propslen + 1));
  char **p = props;
  for (char **e = environ; *e; e++) {
    if (strncmp("IG_APP_", *e, strlen("IG_APP_")) == 0) {
      *p = malloc(1 + strlen(*e) - strlen("IG_APP_") + 1);
      **p = TYPE_APP;
      strcpy(*p + 1, *e + strlen("IG_APP_"));
      char *sep = strchr(*p, '=');
      if (sep) *sep = '\0';
      p++;
    } else if (strncmp("IG_GROUP_", *e, strlen("IG_GROUP_")) == 0) {
      *p = malloc(1 + strlen(*e) - strlen("IG_GROUP_") + 1);
      **p = TYPE_GROUP;
      strcpy(*p + 1, *e + strlen("IG_GROUP_"));
      char *sep = strchr(*p, '=');
      if (sep) *sep = '\0';
      p++;
    } else if (strncmp("IG_APPID=", *e, strlen("IG_APPID=")) == 0) {
      strncpy(app_id, *e + strlen("IG_APPID="), sizeof(app_id) - 1); 
    }
  }
  *p = NULL;
}

void set_args(char **argv) {
  argslen = 0;
  argslen += sizeof("glass-action");
  argslen += sizeof("run");
  argslen += sizeof("-i");
  argslen += sizeof(app_id);

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
  b += sizeof("glass-action");
  strcpy(b, "run");
  b += sizeof("run");
  strcpy(b, "-i");
  b += 3;
  strcpy(b, app_id);
  b += sizeof(app_id);
  for (char **p = props; *p; p++) {
    char *name = *p + 1;
    int namelen = strlen(name);
    char *value = *p + 1 + namelen + 1;
    int valuelen = strlen(value);

    if (**p == TYPE_APP) {
      strcpy(b, "-a");
    } if (**p == TYPE_GROUP) {
      strcpy(b, "-g");
    }
    b += 3;
    strcpy(b, name);
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

Window XCreateWindow(Display *display, Window parent, int x, int y, unsigned int width, unsigned int height, unsigned int border_width,
                     int depth, unsigned int class, Visual *visual, unsigned long valuemask, XSetWindowAttributes *attributes) {
  Window window;

  window = XCreateWindow_orig(display, parent, x, y, width, height, border_width,
                              depth, class, visual, valuemask, attributes);

  Atom prop_type = XInternAtom(display, "STRING", False);
  for (char **p = props; *p; p++) {
    char *name = *p + 1;
    char *value = name + strlen(name) + 1;
    int valuelen = strlen(value);
    if (strcmp(value, "__WM_COMMAND__") == 0) {
      value = args;
      valuelen = argslen;
    } else if (strcmp(value, "__APPID__") == 0) {
      value = app_id;
      valuelen = sizeof(app_id) - 1;
    }
    Atom prop_name = XInternAtom(display, name, False);
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
      if (   (strncmp("IG_APP_", *e_in, strlen("IG_APP_")) != 0)
          && (strcmp("IG_APPID", *e_in) != 0)) {
        e_out++;
      }
    }
    *e_out = NULL;
  }
  return res;
}

#ifndef WM
#define WM

#include "glapi.h"
#include "shader.h"
#include "item.h"
#include "list.h"
#include <pthread.h>

FILE *eventlog;

extern pthread_mutex_t global_lock;


typedef struct {
  Window root, win;
  int    root_x, root_y;
  int    win_x, win_y;
  unsigned int mask;
} Pointer;

extern Pointer mouse;

extern void draw();

#endif

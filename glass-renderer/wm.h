#ifndef WM
#define WM

#include "glapi.h"
#include "shader.h"
#include "item.h"
#include "list.h"

FILE *eventlog;


typedef struct {
  Window root, win;
  int    root_x, root_y;
  int    win_x, win_y;
  unsigned int mask;
} Pointer;

extern Pointer mouse;
extern List *views;
extern List *shaders;

extern void draw();

#endif

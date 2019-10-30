#ifndef WM
#define WM

#include "glapi.h"
#include "shader.h"
#include "item.h"
#include "item_window.h"
#include "list.h"

FILE *eventlog;

extern List *views;
extern List *shaders;

extern void draw();
extern void pick(int x, int y, int *winx, int *winy, Item **item);

#endif

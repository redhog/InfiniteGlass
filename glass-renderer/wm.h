#ifndef WM
#define WM

#include "glapi.h"
#include "shader.h"
#include "item.h"
#include "item_window.h"
#include "list.h"

extern Window motion_notification_window;
extern List *views;

extern void draw();
extern void pick(int x, int y, int *winx, int *winy, Item **item);

#endif

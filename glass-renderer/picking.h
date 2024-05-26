#ifndef PICKING_H
#define PICKING_H

#include "xapi.h"
#include "glapi.h"
#include "view.h"

extern int init_picking();
extern void pick(int x, int y, int *winx, int *winy, Item **item, Item **parent_item);
extern void raw_motion_detected(void);

#endif

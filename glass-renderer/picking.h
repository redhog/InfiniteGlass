#ifndef PICKING_H
#define PICKING_H

#include "item.h"
#include "xapi.h"
#include "glapi.h"

extern void pick(XConnection *conn, int x, int y, int *winx, int *winy, Item **item, Item **parent_item);
extern int init_picking(XConnection *conn);


#endif

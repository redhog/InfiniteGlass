#ifndef ITEM_WINDOW
#define ITEM_WINDOW

#include "item.h"

typedef struct {
  Item base;
} ItemWindow;

extern ItemType item_type_window;

extern void item_type_window_update_space_pos_from_window(Item *item);
extern Item *item_get_from_window(Window window, int create);
extern void items_get_from_toplevel_windows();

#endif

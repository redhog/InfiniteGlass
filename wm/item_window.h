#ifndef ITEM_WINDOW
#define ITEM_WINDOW

#include "item.h"
#include "texture.h"

typedef struct {
  Item base;
 
  Window window;
} ItemWindow;

extern ItemType item_type_window;

extern void item_type_window_update_space_pos_from_window(ItemWindow *item);
extern Item *item_get_from_window(Window window);

#endif

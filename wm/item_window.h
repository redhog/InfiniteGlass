#ifndef ITEM_WINDOW
#define ITEM_WINDOW

#include "item.h"
#include "texture.h"

typedef struct {
  Item base;
 
  Window window;
} ItemWindow;

extern ItemType item_type_window;

Item *item_get_from_window(Window window);

#endif

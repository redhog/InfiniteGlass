#ifndef ITEM_WINDOW
#define ITEM_WINDOW

#include "item.h"
#include "texture.h"

typedef struct {
  Item base;
 
  Window window;

  Damage damage;
 
  Pixmap window_pixmap;
  Texture window_texture;
  Texture icon_texture;
  Texture icon_mask_texture;

  XWMHints wm_hints;
} ItemWindow;

extern ItemType item_type_window;

Item *item_get_from_window(Window window);

#endif
